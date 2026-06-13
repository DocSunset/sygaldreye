// Copyright 2025 Travis West
#pragma once
#include "eyeballs_node_abi.hpp"
#include "gpu_texture.hpp"
#include "subgraph_descriptor_fwd.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
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
    AudioBuffer,
    DrawFn,
    MeshPtr,
    std::string,
    Span
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

// component_registry includes this header; including it back would be circular.
struct ComponentRegistry;
struct TickPlan;

struct Graph {
    std::vector<NodeInstance>                    nodes;
    std::vector<Edge>                            edges;
    std::vector<InletDecl>                       inlets;
    std::vector<OutletDecl>                      outlets;
    std::vector<DrawFn>                          draw_calls;  // cleared each tick
    std::vector<SubgraphDescriptorPtr>           owned_descriptors;
    // Resolved edge references, z⁻¹ delay mappings, region split, and
    // boundary crossings; built lazily on first tick (or by the audio
    // scheduler's rebuild, whichever comes first). Topology never mutates
    // in place (edits swap whole graphs), so the plan lives as long as
    // the graph.
    std::unique_ptr<TickPlan>                    plan;

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

// Adopts live node state from `old` into `fresh` wherever a node keeps its
// id and descriptor across an edit, so live graph updates don't reset the
// world. Params freshly parsed from JSON are re-applied to the adopted
// instance: declarative edits win, internal state survives. The displaced
// fresh instances are destroyed with `old`.
void migrate_graph(Graph& fresh, Graph& old);

// Runs one evaluation step on all nodes in insertion order.
void tick_graph(Graph& g, double time_s);

// Pull observability (endpoints_v6 phase D): one frame-coherent sweep of
// every node's outputs into a fresh map. Execution never builds this —
// it exists only when an observer asks (HTTP /values, probes, tests).
std::unordered_map<std::string, PortValue> snapshot_values(const Graph& g);

// Strips whitespace outside string literals (standard JSON → the compact
// form the mini-parsers expect).
std::string compact_json(const std::string& in);
