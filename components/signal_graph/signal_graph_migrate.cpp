// Copyright 2026 Travis West
#include "signal_graph.hpp"
#include <string>
#include <unordered_map>
#include <utility>

void migrate_graph(Graph& fresh, Graph& old) {
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
