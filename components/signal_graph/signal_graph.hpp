// Copyright 2025 Travis West
#pragma once
#include "eyeballs_node_abi.h"
#include "component_registry.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct NodeInstance {
    const EyeballsNodeDescriptor* desc;
    void*                         data;
    std::string                   id;
};

struct Edge {
    std::string from_node, from_port;
    std::string to_node,   to_port;
};

struct Graph {
    std::vector<NodeInstance>               nodes;
    std::vector<Edge>                       edges;
    std::unordered_map<std::string, double> values;  // "node_id.port_name" → value

    ~Graph();
};

// Parses JSON graph description, instantiates nodes via registry.
// Returns nullptr on error.
std::unique_ptr<Graph> parse_graph(const std::string& json,
                                   const ComponentRegistry& registry);

// Serializes current graph to JSON (node ids + types).
std::string serialize_graph(const Graph& g);

// Runs one evaluation step on all nodes in insertion order.
void tick_graph(Graph& g, double time_s);
