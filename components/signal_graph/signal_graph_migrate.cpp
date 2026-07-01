// Copyright 2026 Travis West
#include "signal_graph.hpp"
#include <string>
#include <unordered_map>
#include <utility>

void migrate_graph(Graph& fresh, Graph& old) {
    // Lifted host instances survive the swap: the fresh plan rebuilds its
    // LiftGroups but reattaches to these by key. Stale entries (host removed
    // / descriptor changed) are reconciled lazily on the next lift tick.
    fresh.lifted_store = std::move(old.lifted_store);

    std::unordered_map<std::string, NodeInstance*> old_by_id;
    for (auto& n : old.nodes) old_by_id[n.id] = &n;

    for (auto& n : fresh.nodes) {
        auto it = old_by_id.find(n.id);
        if (it == old_by_id.end()) continue;
        NodeInstance* o = it->second;
        // Same descriptor pointer guarantees layout compatibility. Inline
        // subgraphs get a new descriptor per parse and are rebuilt instead.
        if (o->desc != n.desc) continue;

        const char* params = nullptr;
        if (n.desc->serialize) params = n.desc->serialize(n.data);
        std::swap(n.data, o->data);
        if (params) {
            if (n.desc->deserialize) n.desc->deserialize(n.data, params);
            if (n.desc->free_str) n.desc->free_str(params);
        }
    }
}
