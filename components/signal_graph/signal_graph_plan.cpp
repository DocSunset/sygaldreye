// Copyright 2026 Travis West
#include "signal_graph_plan.hpp"
#include "signal_graph_sort.hpp"
#include <unordered_map>

namespace {

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

} // namespace

std::unique_ptr<TickPlan> build_plan(const Graph& g) {
    auto back = mark_back_edges(g);
    std::unordered_map<std::string, std::size_t> idx;
    for (std::size_t i = 0; i < g.nodes.size(); ++i) idx[g.nodes[i].id] = i;

    auto plan = std::make_unique<TickPlan>();
    plan->appliers.resize(g.nodes.size());
    plan->delayed.resize(g.nodes.size());

    std::vector<Edge> dag_edges;
    for (std::size_t ei = 0; ei < g.edges.size(); ++ei) {
        const Edge& e = g.edges[ei];
        auto t = idx.find(e.to_node);
        if (t == idx.end() || idx.find(e.from_node) == idx.end()) continue;
        if (back[ei]) {
            plan->delays.push_back(DelayMapping{EdgeApplier{&e}, std::nullopt});
            plan->delayed[t->second].push_back(&plan->delays.back());
        } else {
            plan->appliers[t->second].push_back(EdgeApplier{&e});
            dag_edges.push_back(e);
        }
    }
    plan->order = topo_sort(g.nodes, dag_edges);
    // Nodes owned by another region's scheduler don't tick here.
    if (!g.offrender.empty())
        std::erase_if(plan->order, [&](std::size_t i) {
            return g.offrender.count(g.nodes[i].id) != 0;
        });
    return plan;
}

std::vector<const Edge*> cycle_mappings(const Graph& g) {
    auto back = mark_back_edges(g);
    std::vector<const Edge*> out;
    for (std::size_t ei = 0; ei < g.edges.size(); ++ei)
        if (back[ei]) out.push_back(&g.edges[ei]);
    return out;
}
