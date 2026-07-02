// Copyright 2026 Travis West
#include "signal_graph.hpp"
#include "signal_graph_plan.hpp"
#include <string>
#include <unordered_map>
#include <utility>

void migrate_graph(Graph& fresh, Graph& old) {
    // Lifted host instances survive the swap: the fresh plan rebuilds its
    // LiftGroups and reattaches to these by key. Reconcile NOW, while old's
    // descriptors are still alive (inline-subgraph descs die with `old`):
    // host gone → destroy the entry; host's descriptor changed (re-parsed
    // inline subgraph, re-registered type) → rebuild the instance on the new
    // desc, carrying serialized state. Entries whose lift EDGE went away
    // (host kept) are dropped by wire_plan once the fresh plan is built.
    fresh.lifted_store = std::move(old.lifted_store);
    for (auto it = fresh.lifted_store.begin(); it != fresh.lifted_store.end();) {
        const NodeInstance* host = nullptr;
        for (const auto& n : fresh.nodes)  // key = "hostid.in_port#…"
            if (it->first.rfind(n.id + ".", 0) == 0) {
                host = &n;
                break;
            }
        if (!host) {
            if (it->second.desc && it->second.desc->destroy && it->second.data)
                it->second.desc->destroy(it->second.data);
            it = fresh.lifted_store.erase(it);
            continue;
        }
        if (host->desc != it->second.desc) migrate_lifted_instance(it->second, host->desc);
        ++it;
    }

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
