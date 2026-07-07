// Copyright 2025 Travis West
#include "signal_graph_sort.hpp"
#include <algorithm>
#include <queue>
#include <unordered_map>

std::vector<std::size_t> topo_sort(const std::vector<NodeInstance>& nodes,
                                   const std::vector<Edge>& edges) {
    std::unordered_map<std::string, std::size_t> idx_map;
    idx_map.reserve(nodes.size());
    for (std::size_t i = 0; i < nodes.size(); ++i)
        idx_map[nodes[i].id] = i;

    std::vector<int> in_deg(nodes.size(), 0);
    for (const auto& e : edges) {
        auto it_to   = idx_map.find(e.to_node);
        auto it_from = idx_map.find(e.from_node);
        if (it_to == idx_map.end() || it_from == idx_map.end()) continue;
        ++in_deg[it_to->second];
    }

    std::queue<std::size_t> q;
    for (std::size_t i = 0; i < nodes.size(); ++i)
        if (in_deg[i] == 0) q.push(i);

    std::vector<std::size_t> order;
    order.reserve(nodes.size());
    while (!q.empty()) {
        auto i = q.front(); q.pop();
        order.push_back(i);
        for (const auto& e : edges) {
            if (e.from_node != nodes[i].id) continue;
            auto it = idx_map.find(e.to_node);
            if (it == idx_map.end()) continue;
            if (--in_deg[it->second] == 0) q.push(it->second);
        }
    }
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (std::find(order.begin(), order.end(), i) == order.end())
            order.push_back(i);
    }
    return order;
}
