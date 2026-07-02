// Copyright 2026 Travis West
#include <cstdio>

#include "signal_graph.hpp"
#include "signal_graph_plan.hpp"

// ── lift replay (conformability.md "lift frames") ───────────────────────────
// One implementation for every lifted host: reconcile N instances against the
// live array (keyed by cell value when the host declares a lift_key, else by
// index — so state survives a reorder), feed each row, tick, gather outputs.

namespace {

// Per-row key: from the SEPARATE key Span when one is wired (card keys on its
// stable id, not the lifted cell), else the lifted cell itself (L1), else the
// row index. Keying on a stable source is what lets a card keep its lifted
// state when the array reorders or the graph is edited live. %.9g: floats
// round-trip (graph_source keys are 24-bit hashes; %g would truncate them
// into collisions).
std::string row_key(const LiftGroup& lg, const Span& cell, const Span& key, int r) {
    if (lg.key_port.empty()) return std::to_string(r);
    const Span& s = (key.data && lg.key_port != lg.in_port) ? key : cell;
    if (!s.data) return std::to_string(r);
    std::string k;
    for (int c = 0; c < s.cols; ++c) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.9g,", double(s.data[std::size_t(r) * s.cols + c]));
        k += buf;
    }
    return k;
}

// Copy a lifted instance's gathered output into the group's gather slot.
void gather_row(LiftGroup& lg, const NodeInstance& inst, int /*row*/) {
    if (lg.out_kind == "mesh") {
        auto v = read_output(inst, lg.out_port, "mesh");
        lg.mesh_gather.push_back(v ? std::get<MeshPtr>(*v) : nullptr);
        return;
    }
    auto v = read_output(inst, lg.out_port, lg.out_kind);
    auto push = [&](std::initializer_list<float> fs) {
        for (float f : fs) lg.gather_buf.push_back(f);
    };
    if (!v) {
        // Zero-fill one OUTPUT row (in-cell and out-cell ranks can differ).
        for (int i = 0; i < lg.out_cell; ++i) lg.gather_buf.push_back(0.f);
        return;
    }
    std::visit(
        [&](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, double>)
                push({float(val)});
            else if constexpr (std::is_same_v<T, Eigen::Vector2f>)
                push({val.x(), val.y()});
            else if constexpr (std::is_same_v<T, Eigen::Vector3f>)
                push({val.x(), val.y(), val.z()});
            else if constexpr (std::is_same_v<T, Eigen::Vector4f>)
                push({val.x(), val.y(), val.z(), val.w()});
            else if constexpr (std::is_same_v<T, Eigen::Quaternionf>)
                push({val.x(), val.y(), val.z(), val.w()});
            else if constexpr (std::is_same_v<T, Eigen::Matrix4f>)
                for (int i = 0; i < 16; ++i) lg.gather_buf.push_back(val.data()[i]);
        },
        *v);
}

// Bind row r's cell to the lifted input. Eigen vec/mat are contiguous
// floats, so a row pointer wires straight in.
void bind_row(NodeInstance& inst, const LiftGroup& lg, const Span& src, int r) {
    if (inst.desc->connect)
        inst.desc->connect(
            inst.data,
            lg.in_port.c_str(),
            static_cast<const void*>(src.data + std::size_t(r) * lg.cell));
}

// Bind row r's key cell to the lifted key input (so a keyed subgraph instance
// receives its id alongside its lifted cell). Only when a separate key source
// is wired (key_port != in_port).
void bind_key(NodeInstance& inst, const LiftGroup& lg, const Span& key, int r) {
    if (key.data && lg.key_port != lg.in_port && inst.desc->connect)
        inst.desc->connect(
            inst.data,
            lg.key_port.c_str(),
            static_cast<const void*>(key.data + std::size_t(r) * lg.key_cell));
}

// Clones: reconcile N stored instances against the live array (keyed by the
// separate key Span / cell value when the host declares a lift_key, else by
// index — state survives a reorder), feed each row, tick, gather.
void tick_clones(
    Graph& g, LiftGroup& lg, const Span& src, const Span& key, int rows, double time_s) {
    const NodeInstance& host = g.nodes[lg.host];
    lg.instances.clear();
    lg.instances.reserve(std::size_t(rows));
    std::vector<std::string> live;
    live.reserve(std::size_t(rows));
    for (int r = 0; r < rows; ++r) {
        std::string sk = lg.store_prefix + row_key(lg, src, key, r);
        auto it = g.lifted_store.find(sk);
        if (it == g.lifted_store.end()) {
            LiftedInstance li{host.desc, host.desc->create()};
            // host params (and inner wiring for subgraphs) come from create();
            // copy the host's serialized params so clones match the template.
            if (host.desc->serialize && host.desc->deserialize && li.data) {
                const char* s = host.desc->serialize(host.data);
                host.desc->deserialize(li.data, s);
                if (host.desc->free_str) host.desc->free_str(s);
            }
            it = g.lifted_store.emplace(sk, li).first;
        } else if (it->second.desc != host.desc) {
            // The host's definition changed under the key (hot reload /
            // re-registered subgraph): rebuild on the CURRENT desc — never
            // run the stale one.
            migrate_lifted_instance(it->second, host.desc);
        }
        live.push_back(std::move(sk));
        lg.instances.push_back({it->second.desc, it->second.data, it->first});
    }
    // Drop store entries no longer live (array shrank / keys changed). Skip
    // the scan while the key set is unchanged — the common frame.
    if (!lg.reconciled || live != lg.inst_keys) {
        std::unordered_set<std::string> keep(live.begin(), live.end());
        for (auto it = g.lifted_store.begin(); it != g.lifted_store.end();) {
            bool mine = it->first.rfind(lg.store_prefix, 0) == 0;
            if (mine && !keep.count(it->first)) {
                if (it->second.desc->destroy) it->second.desc->destroy(it->second.data);
                it = g.lifted_store.erase(it);
            } else
                ++it;
        }
        lg.inst_keys = std::move(live);
        lg.reconciled = true;
    }
    for (int r = 0; r < rows; ++r) {
        NodeInstance& inst = lg.instances[std::size_t(r)];
        bind_row(inst, lg, src, r);
        bind_key(inst, lg, key, r);
        if (inst.desc->process) inst.desc->process(inst.data, time_s);
        gather_row(lg, inst, r);
    }
}

// CellMap: a stateless host — loop the ONE host (the live node) over rows, no
// per-row state. No store, no clones (the whole point of declaring stateless).
void tick_cellmap(
    Graph& g, LiftGroup& lg, const Span& src, const Span& key, int rows, double time_s) {
    NodeInstance host = g.nodes[lg.host];  // copy: desc + data, not owned
    for (int r = 0; r < rows; ++r) {
        bind_row(host, lg, src, r);
        bind_key(host, lg, key, r);
        if (host.desc->process) host.desc->process(host.data, time_s);
        gather_row(lg, host, r);
    }
}

}  // namespace

void migrate_lifted_instance(LiftedInstance& li, const EyeballsNodeDescriptor* desc) {
    LiftedInstance fresh{desc, desc->create()};
    if (li.desc && li.desc->serialize && desc->deserialize && li.data && fresh.data) {
        const char* s = li.desc->serialize(li.data);
        desc->deserialize(fresh.data, s);
        if (li.desc->free_str) li.desc->free_str(s);
    }
    if (li.desc && li.desc->destroy && li.data) li.desc->destroy(li.data);
    li = fresh;
}

void tick_lift_group(Graph& g, LiftGroup& lg, double time_s) {
    Span src{};
    if (auto v = read_output(lg.source); v && std::holds_alternative<Span>(*v))
        src = std::get<Span>(*v);
    Span key{};
    if (lg.key_source.from)
        if (auto v = read_output(lg.key_source); v && std::holds_alternative<Span>(*v))
            key = std::get<Span>(*v);
    int rows = (src.data && src.cols == lg.cell) ? src.rows : 0;

    lg.gather_buf.clear();
    lg.mesh_gather.clear();
    if (lg.strategy == LiftGroup::CellMap)
        tick_cellmap(g, lg, src, key, rows, time_s);
    else
        tick_clones(g, lg, src, key, rows, time_s);
    lg.gather = Span{
        lg.gather_buf.empty() ? nullptr : lg.gather_buf.data(),
        rows,
        lg.out_cell,
        Axis::Item,
        Axis::Cell};
    // mesh out (forest route 1): N distinct meshes as a borrowed array view.
    lg.mesh_view = MeshArray{
        lg.mesh_gather.empty() ? nullptr : lg.mesh_gather.data(), int(lg.mesh_gather.size())};
}
