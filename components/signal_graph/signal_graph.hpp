// Copyright 2025 Travis West
#pragma once
#include "eyeballs_node_abi.hpp"
#include "gpu_texture.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

using PortValue = std::variant<
    double,
    Eigen::Vector2f,
    Eigen::Vector3f,
    Eigen::Vector4f,
    Eigen::Matrix4f,
    Eigen::Quaternionf,
    GpuTexture,
    AudioBuffer
>;

struct NodeInstance {
    const EyeballsNodeDescriptor* desc;
    void*                         data;
    std::string                   id;
};

struct Edge {
    std::string from_node, from_port;
    std::string to_node,   to_port;
};

struct InletDecl  { std::string name, node, port; };
struct OutletDecl { std::string name, node, port; };

// Forward declarations — subgraph_node and component_registry both include
// this header; including either back would be circular.
struct SubgraphDescriptor;
struct ComponentRegistry;

struct Graph {
    std::vector<NodeInstance>                    nodes;
    std::vector<Edge>                            edges;
    std::vector<InletDecl>                       inlets;
    std::vector<OutletDecl>                      outlets;
    std::unordered_map<std::string, PortValue>   values;      // "node_id.port_name" → typed value
    std::vector<DrawFn>                          draw_calls;  // cleared each tick
    std::vector<std::unique_ptr<SubgraphDescriptor>> owned_descriptors;

    ~Graph();
};

// Deep-clones a graph: re-instantiates each node via desc->create() and
// serialize/deserialize, copies edges, inlets, outlets. owned_descriptors are
// not cloned (only valid for templates of non-subgraph nodes).
std::unique_ptr<Graph> clone_graph(const Graph& src);

// Parses JSON graph description, instantiates nodes via registry.
// Returns nullptr on error.
std::unique_ptr<Graph> parse_graph(const std::string& json,
                                   const ComponentRegistry& registry);

// Serializes current graph to JSON (node ids + types).
std::string serialize_graph(const Graph& g);

// Runs one evaluation step on all nodes in insertion order.
void tick_graph(Graph& g, double time_s);
