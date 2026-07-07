// Copyright 2026 Travis West
#include "signal_graph_plan.hpp"

#include <cstdio>
#include <string_view>
#include <unordered_map>

#include "port_schema_reader.hpp"
#include "signal_graph_sort.hpp"

namespace {

using port_types::Rate;

// DFS over the whole graph in node order; an edge whose target is on the
// DFS stack closes a cycle. Removing every back edge yields a DAG.
std::vector<char> mark_back_edges(const Graph& g) {
    std::unordered_map<std::string, std::size_t> idx;
    for (std::size_t i = 0; i < g.nodes.size(); ++i) idx[g.nodes[i].id] = i;

    std::vector<std::vector<std::size_t>> out(g.nodes.size());
    for (std::size_t ei = 0; ei < g.edges.size(); ++ei) {
        auto f = idx.find(g.edges[ei].from_node), t = idx.find(g.edges[ei].to_node);
        if (f != idx.end() && t != idx.end()) out[f->second].push_back(ei);
    }

    std::vector<char> back(g.edges.size(), 0);
    std::vector<char> state(g.nodes.size(), 0);              // 0 new, 1 on stack, 2 done
    std::vector<std::pair<std::size_t, std::size_t>> stack;  // node, next child
    for (std::size_t root = 0; root < g.nodes.size(); ++root) {
        if (state[root]) continue;
        stack.push_back({root, 0});
        state[root] = 1;
        while (!stack.empty()) {
            auto& [n, child] = stack.back();
            if (child >= out[n].size()) {
                state[n] = 2;
                stack.pop_back();
                continue;
            }
            std::size_t ei = out[n][child++];
            std::size_t to = idx[g.edges[ei].to_node];
            if (state[to] == 1)
                back[ei] = 1;
            else if (state[to] == 0) {
                state[to] = 1;
                stack.push_back({to, 0});
            }
        }
    }
    return back;
}

std::string out_kind(const NodeInstance& n, const std::string& port) {
    PortSchema s = parse_port_schema(n.desc ? n.desc->port_schema : nullptr);
    for (const auto& p : s.outputs)
        if (p.name == port) return p.kind;
    return "unknown";
}

// The consumer input port's schema entry (kind/cell_rank/whole), if listed.
std::optional<PortInfo> in_info(const NodeInstance& n, const std::string& port) {
    PortSchema s = parse_port_schema(n.desc ? n.desc->port_schema : nullptr);
    for (const auto& p : s.inputs)
        if (p.name == port) return p;
    return std::nullopt;
}

// The lifted host's SINGLE gathered output: a node lifted over an array
// produces one output per instance, gathered downstream. We gather the
// first listed output (lifted generators/subgraphs have one meaningful out;
// the editor card's outlet is its draw/mesh). kind drives the gather slot.
std::pair<std::string, std::string> host_gather_out(const NodeInstance& n) {
    PortSchema s = parse_port_schema(n.desc ? n.desc->port_schema : nullptr);
    if (!s.outputs.empty()) return {s.outputs.front().name, s.outputs.front().kind};
    return {"", ""};
}

// Excess rank: producer emits a span, the consumer port is a cell-kind
// (rank>=1) that does NOT opt out of lifting. The actual cols/axis match is
// checked at tick time against the live Span (rows/cols/axis are runtime).
bool is_excess_rank_edge(
    const NodeInstance& from,
    const std::string& from_port,
    const NodeInstance& to,
    const std::string& to_port) {
    if (out_kind(from, from_port) != "span") return false;
    auto info = in_info(to, to_port);
    if (!info) return false;
    return info->cell_rank() >= 1 && !info->whole;
}

// Block region (v1 rule, see header): dacs + upstream closure through
// audio edges.
std::vector<Rate> infer_regions(
    const Graph& g, const std::unordered_map<std::string, std::size_t>& idx) {
    std::vector<Rate> region(g.nodes.size(), Rate::Frame);
    std::vector<std::size_t> frontier;
    for (std::size_t i = 0; i < g.nodes.size(); ++i)
        if (std::string_view{g.nodes[i].desc->type_name} == "dac") {
            region[i] = Rate::Block;
            frontier.push_back(i);
        }
    while (!frontier.empty()) {
        std::size_t to = frontier.back();
        frontier.pop_back();
        for (const auto& e : g.edges) {
            if (e.to_node != g.nodes[to].id) continue;
            auto f = idx.find(e.from_node);
            if (f == idx.end() || region[f->second] == Rate::Block) continue;
            if (out_kind(g.nodes[f->second], e.from_port) != "audio") continue;
            region[f->second] = Rate::Block;
            frontier.push_back(f->second);
        }
    }
    return region;
}

}  // namespace

std::unique_ptr<TickPlan> build_plan(const Graph& g) {
    auto back = mark_back_edges(g);
    std::unordered_map<std::string, std::size_t> idx;
    for (std::size_t i = 0; i < g.nodes.size(); ++i) idx[g.nodes[i].id] = i;

    auto plan = std::make_unique<TickPlan>();
    plan->node_region = infer_regions(g, idx);
    plan->appliers.resize(g.nodes.size());
    plan->slot_appliers.resize(g.nodes.size());
    plan->delayed.resize(g.nodes.size());
    auto make_applier = [&](const Edge& e, std::size_t from_i) {
        return EdgeApplier{&e, &g.nodes[from_i], out_kind(g.nodes[from_i], e.from_port)};
    };

    // ── lift pre-pass (conformability.md): detect excess-rank edges ──────────
    // host node index → its LiftGroup (a host lifts on ONE array input here;
    // multi-input lift is a later rung). The host node is then a LIFTED slot:
    // it is NOT ticked in place, and edges from its output gather.
    std::unordered_map<std::size_t, LiftGroup*> host_group;
    // The lift_key port is the KEY channel (id), not the lift trigger. A span
    // feeding it is excess-rank too, so lifting on it (if its edge is listed
    // first) would bind the id per row and leave the real cell unbound. Skip
    // it — UNLESS it is the host's ONLY excess-rank input (the L1 case where
    // key and cell are the same port). Find such hosts up front.
    auto has_other_excess = [&](std::size_t host, std::string_view key_port) {
        for (const auto& e : g.edges) {
            auto f = idx.find(e.from_node), t = idx.find(e.to_node);
            if (f == idx.end() || t->second != host || e.to_port == key_port) continue;
            if (is_excess_rank_edge(g.nodes[f->second], e.from_port, g.nodes[host], e.to_port))
                return true;
        }
        return false;
    };
    for (const auto& e : g.edges) {
        auto f = idx.find(e.from_node), t = idx.find(e.to_node);
        if (f == idx.end() || t == idx.end()) continue;
        if (host_group.count(t->second)) continue;  // one lift per host (v1)
        const NodeInstance& from = g.nodes[f->second];
        const NodeInstance& to = g.nodes[t->second];
        if (to.desc->lift_key && std::string_view{to.desc->lift_key} == e.to_port &&
            has_other_excess(t->second, to.desc->lift_key))
            continue;  // lift on the cell input, not the id channel
        if (!is_excess_rank_edge(from, e.from_port, to, e.to_port)) continue;
        // Resource-holder guard: lifting a device/stream/editor node (or a
        // subgraph containing one) is a hard error. Drop the lift; the edge
        // falls through to ordinary classification, which is itself a type
        // mismatch (span→cell) — loud where it lands.
        if (to.desc->lift_kind == EYEBALLS_LIFT_RESOURCE_HOLDER) {
            std::fprintf(
                stderr,
                "build_plan: cannot lift resource-holder '%s' (node '%s') over an "
                "array — resource holders (dac/editor/device streams) are unliftable\n",
                to.desc->type_name ? to.desc->type_name : "?",
                to.id.c_str());
            continue;
        }
        LiftGroup lg;
        lg.host = t->second;
        lg.in_port = e.to_port;
        lg.source = make_applier(e, f->second);
        if (auto info = in_info(to, e.to_port)) lg.cell = info->cell_rank();
        // Stateless hosts loop in place (CellMap); the safe default is N
        // migrated stateful instances (Clones).
        lg.strategy =
            to.desc->lift_kind == EYEBALLS_LIFT_STATELESS ? LiftGroup::CellMap : LiftGroup::Clones;
        auto [op, ok] = host_gather_out(to);
        lg.out_port = op;
        lg.out_kind = ok;
        if (auto oi = parse_port_schema(to.desc->port_schema); !oi.outputs.empty())
            lg.out_cell = oi.outputs.front().cell_rank();
        if (to.desc->lift_key) lg.key_port = to.desc->lift_key;
        // Separate key source: when lift_key names a DIFFERENT input than the
        // lifted one, the Span edge feeding it is the key channel (id), read
        // per row instead of the lifted cell. (key_port == in_port keeps the
        // L1 behavior: key on the cell value itself.)
        if (!lg.key_port.empty() && lg.key_port != lg.in_port)
            for (const auto& ke : g.edges) {
                if (ke.to_node != to.id || ke.to_port != lg.key_port) continue;
                auto kf = idx.find(ke.from_node);
                if (kf == idx.end()) continue;
                lg.key_source = make_applier(ke, kf->second);
                if (auto ki = in_info(to, lg.key_port)) lg.key_cell = ki->cell_rank();
                break;
            }
        plan->lift_groups.push_back(std::move(lg));
        host_group[t->second] = &plan->lift_groups.back();
    }

    std::vector<Edge> dag_edges;
    for (std::size_t ei = 0; ei < g.edges.size(); ++ei) {
        const Edge& e = g.edges[ei];
        auto f = idx.find(e.from_node), t = idx.find(e.to_node);
        if (t == idx.end() || f == idx.end()) continue;
        // Lift routing: the array edge INTO a lifted host is consumed by the
        // LiftGroup (keep it as a dag_edge so the host orders after the
        // producer, but no wire/applier). The key-source edge (lift_key port)
        // is likewise consumed by the group. An edge FROM a lifted host's
        // gather output points the consumer at the group's gather slot.
        if (auto hg = host_group.find(t->second);
            hg != host_group.end() &&
            (hg->second->in_port == e.to_port || hg->second->key_port == e.to_port)) {
            dag_edges.push_back(e);
            continue;
        }
        if (auto hg = host_group.find(f->second);
            hg != host_group.end() && hg->second->out_port == e.from_port) {
            plan->gather_wires.push_back({&e, hg->second});
            dag_edges.push_back(e);
            continue;
        }
        Rate rf = plan->node_region[f->second], rt = plan->node_region[t->second];
        if (rf != rt) {
            // Region crossing: reified as a canonical boundary mapping.
            Crossing c;
            c.edge = &e;
            c.payload_kind = out_kind(g.nodes[f->second], e.from_port);
            c.mapping = port_types::crossing_mapping(c.payload_kind, rf, rt);
            c.from_region = rf;
            c.to_region = rt;
            plan->crossings.push_back(std::move(c));
            // A mapping INTO the frame region terminates in a values slot
            // (ring/snapshot publish there). The final hop: stream payloads
            // into a v6 consumer go through a plan-owned typed slot the
            // consumer's src points at; everything else is an ordinary
            // applier. Mappings into the block region (latch) apply
            // themselves on the audio thread.
            // Frame-side delivery is the MAPPING's job (audio_region
            // publish applies scalars/events to the consumer directly;
            // rings fill the plan-owned slot the consumer's src points at).
            if (rt == Rate::Frame) {
                std::string kind = out_kind(g.nodes[f->second], e.from_port);
                if (g.nodes[t->second].desc->connect && kind == "audio") {
                    TickPlan::SlotApplier sa{make_applier(e, f->second), nullptr};
                    plan->audio_slots.emplace_back();
                    sa.audio = &plan->audio_slots.back();
                    plan->slot_appliers[t->second].push_back(sa);
                }
            }
            continue;
        }
        if (back[ei]) {
            plan->delays.push_back(DelayMapping{make_applier(e, f->second), std::nullopt, rf});
            plan->delayed[t->second].push_back(&plan->delays.back());
        } else if (
            g.nodes[f->second].desc->version >= 6 && g.nodes[t->second].desc->version >= 6 &&
            g.nodes[f->second].desc->output_ptr && g.nodes[t->second].desc->connect &&
            out_kind(g.nodes[f->second], e.from_port) != "bang") {
            // endpoints v6 both sides: a literal pointer, wired once.
            // (Bangs are DELIVERIES, not values — they keep the applier
            // path: emitted as 0/1 scalars, reset by the producer.)
            plan->wires.push_back(&e);
            dag_edges.push_back(e);
        } else {
            plan->appliers[t->second].push_back(make_applier(e, f->second));
            dag_edges.push_back(e);
        }
    }

    auto full_order = topo_sort(g.nodes, dag_edges);
    for (std::size_t i : full_order) {
        if (plan->node_region[i] == Rate::Block)
            plan->block_order.push_back(i);
        else
            plan->order.push_back(i);
    }
    return plan;
}

void wire_plan(Graph& g) {
    if (!g.plan) return;
    std::unordered_map<std::string, NodeInstance*> by_id;
    for (auto& n : g.nodes) by_id[n.id] = &n;
    // Reset first: migrated instances may hold src pointers into producers
    // the swap just destroyed.
    for (auto& n : g.nodes) {
        if (n.desc->version < 6 || !n.desc->connect) continue;
        PortSchema s = parse_port_schema(n.desc->port_schema);
        for (const auto& p : s.inputs) n.desc->connect(n.data, p.name.c_str(), nullptr);
    }
    // Crossing slots: point each v6 consumer at its plan-owned slot.
    for (std::size_t i = 0; i < g.plan->slot_appliers.size(); ++i)
        for (auto& sa : g.plan->slot_appliers[i])
            g.nodes[i].desc->connect(
                g.nodes[i].data,
                sa.applier.edge->to_port.c_str(),
                static_cast<const void*>(sa.audio));
    for (const Edge* e : g.plan->wires) {
        auto f = by_id.find(e->from_node), t = by_id.find(e->to_node);
        if (f == by_id.end() || t == by_id.end()) continue;
        const void* src = f->second->desc->output_ptr(f->second->data, e->from_port.c_str());
        if (src) t->second->desc->connect(t->second->data, e->to_port.c_str(), src);
    }
    // Gather wires: numeric-cell lifts gather rows into a plan-owned Span;
    // mesh lifts gather N distinct meshes into a plan-owned MeshArray view.
    // Either way the downstream consumer's src points at the plan-owned slot.
    for (auto& gw : g.plan->gather_wires) {
        auto t = by_id.find(gw.edge->to_node);
        if (t == by_id.end() || !t->second->desc->connect) continue;
        const void* slot = gw.group->out_kind == "mesh"
                               ? static_cast<const void*>(&gw.group->mesh_view)
                               : static_cast<const void*>(&gw.group->gather);
        t->second->desc->connect(t->second->data, gw.edge->to_port.c_str(), slot);
    }
}

std::vector<const Edge*> cycle_mappings(const Graph& g) {
    auto back = mark_back_edges(g);
    std::vector<const Edge*> out;
    for (std::size_t ei = 0; ei < g.edges.size(); ++ei)
        if (back[ei]) out.push_back(&g.edges[ei]);
    return out;
}
