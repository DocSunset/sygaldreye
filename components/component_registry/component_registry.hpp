// Copyright 2025 Travis West
#pragma once
#include "eyeballs_node_abi.h"
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
    bool load_plugin(const std::string& path);

    // Look up a type by name; returns nullptr if not found.
    const EyeballsNodeDescriptor* find(const std::string& type_name) const;

    // All registered type names.
    std::vector<std::string> type_names() const;

    ~ComponentRegistry();

private:
    std::unordered_map<std::string, RegistryEntry> entries_;
};
