// Copyright 2025 Travis West
#pragma once
#include "eyeballs_node_abi.h"
#include "subgraph_descriptor_fwd.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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
    std::vector<SubgraphDescriptorPtr>                  subgraph_descriptors_;
    // Hot-reload: replaced entries retire here and their handles stay open
    // for the life of the registry — live node instances still execute code
    // from the old .so until the next graph swap re-instantiates them, and
    // dlclose before that is a use-after-unload. The leak is deliberate and
    // bounded by the number of reloads.
    std::vector<RegistryEntry>                          retired_;
};
