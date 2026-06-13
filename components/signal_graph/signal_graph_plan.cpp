// Copyright 2026 Travis West
#include "signal_graph_plan.hpp"
#include "signal_graph_sort.hpp"
#include "port_schema_reader.hpp"
#include <string_view>
#include <unordered_map>

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
    std::vector<char> state(g.nodes.size(), 0);  // 0 new, 1 on stack, 2 done
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
            if (state[to] == 1)      back[ei] = 1;
            else if (state[to] == 0) { state[to] = 1; stack.push_back({to, 0}); }
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

// Block region (v1 rule, see header): dacs + upstream closure through
// audio edges.
std::vector<Rate> infer_regions(const Graph& g,
                                const std::unordered_map<std::string, std::size_t>& idx) {
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

} // namespace

std::unique_ptr<TickPlan> build_plan(const Graph& g) {
    auto back = mark_back_edges(g);
    std::unordered_map<std::string, std::size_t> idx;
    for (std::size_t i = 0; i < g.nodes.size(); ++i) idx[g.nodes[i].id] = i;

    auto plan = std::make_unique<TickPlan>();
    plan->node_region = infer_regions(g, idx);
    plan->appliers.resize(g.nodes.size());
    plan->slot_appliers.resize(g.nodes.size());
    plan->delayed.resize(g.nodes.size());
    plan->draw_consumed.assign(g.nodes.size(), 0);
    auto make_applier = [&](const Edge& e, std::size_t from_i) {
        return EdgeApplier{&e, &g.nodes[from_i],
                           out_kind(g.nodes[from_i], e.from_port)};
    };
    for (const auto& e : g.edges) {
        auto f = idx.find(e.from_node);
        if (f != idx.end() &&
            out_kind(g.nodes[f->second], e.from_port) == "draw_call")
            plan->draw_consumed[f->second] = 1;
    }

    std::vector<Edge> dag_edges;
    for (std::size_t ei = 0; ei < g.edges.size(); ++ei) {
        const Edge& e = g.edges[ei];
        auto f = idx.find(e.from_node), t = idx.find(e.to_node);
        if (t == idx.end() || f == idx.end()) continue;
        Rate rf = plan->node_region[f->second], rt = plan->node_region[t->second];
        if (rf != rt) {
            // Region crossing: reified as a canonical boundary mapping.
            Crossing c;
            c.edge         = &e;
            c.payload_kind = out_kind(g.nodes[f->second], e.from_port);
            c.mapping      = port_types::crossing_mapping(c.payload_kind, rf, rt);
            c.from_region  = rf;
            c.to_region    = rt;
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
            plan->delays.push_back(DelayMapping{make_applier(e, f->second),
                                                std::nullopt, rf});
            plan->delayed[t->second].push_back(&plan->delays.back());
        } else if (g.nodes[f->second].desc->version >= 6 &&
                   g.nodes[t->second].desc->version >= 6 &&
                   g.nodes[f->second].desc->output_ptr &&
                   g.nodes[t->second].desc->connect &&
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
        if (plan->node_region[i] == Rate::Block) plan->block_order.push_back(i);
        else                                     plan->order.push_back(i);
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
        for (const auto& p : s.inputs)
            n.desc->connect(n.data, p.name.c_str(), nullptr);
    }
    // Crossing slots: point each v6 consumer at its plan-owned slot.
    for (std::size_t i = 0; i < g.plan->slot_appliers.size(); ++i)
        for (auto& sa : g.plan->slot_appliers[i])
            g.nodes[i].desc->connect(
                g.nodes[i].data, sa.applier.edge->to_port.c_str(),
                static_cast<const void*>(sa.audio));
    for (const Edge* e : g.plan->wires) {
        auto f = by_id.find(e->from_node), t = by_id.find(e->to_node);
        if (f == by_id.end() || t == by_id.end()) continue;
        const void* src = f->second->desc->output_ptr(f->second->data,
                                                      e->from_port.c_str());
        if (src) t->second->desc->connect(t->second->data, e->to_port.c_str(), src);
    }
}

std::vector<const Edge*> cycle_mappings(const Graph& g) {
    auto back = mark_back_edges(g);
    std::vector<const Edge*> out;
    for (std::size_t ei = 0; ei < g.edges.size(); ++ei)
        if (back[ei]) out.push_back(&g.edges[ei]);
    return out;
}
