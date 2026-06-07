// Copyright 2025 Travis West
#pragma once
#include "signal_graph.hpp"
#include "eyeballs_node_abi.h"
#include <memory>
#include <string>
#include <unordered_map>

// A node whose implementation is itself a Graph (a Pure Data-style subpatch).
// Inlet values are cached by the parent, injected into internal nodes, then the
// inner graph is ticked; outlet values are read back from the inner value store.
class SubgraphNode {
public:
    explicit SubgraphNode(std::unique_ptr<Graph> inner);
    void operator()(double t);
    void cache_inlet(const std::string& name, PortValue val);
    void push_outlets(EyeballsOutputCtx* ctx) const;
    void push_draw_calls_to(DrawCallCtx* ctx);

private:
    std::unique_ptr<Graph>                     inner_;
    std::unordered_map<std::string, PortValue> inlet_cache_;
};

// Owns a dynamically-allocated EyeballsNodeDescriptor and its heap strings for a
// single subgraph type. create() deep-clones the template into a SubgraphNode.
struct SubgraphDescriptor {
    explicit SubgraphDescriptor(std::unique_ptr<Graph> graph_template,
                                std::string type_name);
    ~SubgraphDescriptor();
    SubgraphDescriptor(const SubgraphDescriptor&)            = delete;
    SubgraphDescriptor& operator=(const SubgraphDescriptor&) = delete;

    const EyeballsNodeDescriptor* descriptor() const { return &desc_; }
    Graph* template_ptr() const { return graph_template_.get(); }

private:
    std::unique_ptr<Graph> graph_template_;
    std::string            type_name_;
    std::string            port_schema_;
    EyeballsNodeDescriptor desc_{};
    int                    slot_ = -1;
};
