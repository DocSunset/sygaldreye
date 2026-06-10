// Copyright 2025 Travis West
#pragma once
#include "eyeballs_node_abi.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct SubgraphDescriptor;

struct RegistryEntry {
    const EyeballsNodeDescriptor* desc;
    void*                         handle;  // dlopen handle, nullptr for built-ins
};

struct ComponentRegistry {
    // Register a statically-linked built-in node type.
    void register_builtin(const EyeballsNodeDescriptor* desc);

    // Load a plugin .so and register it; returns false on failure.
    // If path ends in ".json", parses it as a subgraph definition instead.
    bool load_plugin(const std::string& path);

    // Parse and register a subgraph JSON file; returns false on failure.
    bool load_subgraph_json(const std::string& path);

    // Look up a type by name; returns nullptr if not found.
    const EyeballsNodeDescriptor* find(const std::string& type_name) const;

    // All registered type names.
    std::vector<std::string> type_names() const;

    ~ComponentRegistry();

private:
    std::unordered_map<std::string, RegistryEntry>      entries_;
    std::vector<std::unique_ptr<SubgraphDescriptor>>    subgraph_descriptors_;
};
