// Copyright 2025 Travis West
#include "component_registry.hpp"
#include "signal_graph.hpp"
#include <cstdio>
#include <dlfcn.h>

#ifdef __ANDROID__
#include <android/log.h>
#define LOG(...)  __android_log_print(ANDROID_LOG_INFO,  "eyeballs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)
#else
#define LOG(fmt, ...)  std::fprintf(stdout, "[INFO]  " fmt "\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) std::fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#endif

using DescriptorFn = const EyeballsNodeDescriptor*(*)();

void ComponentRegistry::register_builtin(const EyeballsNodeDescriptor* desc) {
    if (!desc || !desc->type_name) return;
    entries_[desc->type_name] = RegistryEntry{desc, nullptr};
}

bool ComponentRegistry::load_subgraph_json(const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        LOGE("component_registry: cannot open %s", path.c_str());
        return false;
    }
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::rewind(f);
    std::string contents(static_cast<std::size_t>(sz), '\0');
    std::fread(contents.data(), 1, static_cast<std::size_t>(sz), f);
    std::fclose(f);

    // Derive type name from file stem.
    std::size_t slash = path.find_last_of("/\\");
    std::string stem = (slash == std::string::npos) ? path : path.substr(slash + 1);
    // Strip ".json" suffix (5 chars).
    std::string type_name = stem.substr(0, stem.size() - 5);

    auto graph = parse_graph(contents, *this);
    if (!graph) {
        LOGE("component_registry: failed to parse subgraph JSON %s", path.c_str());
        return false;
    }
    auto desc = std::make_unique<SubgraphDescriptor>(std::move(graph), type_name);
    register_builtin(desc->descriptor());
    subgraph_descriptors_.push_back(std::move(desc));
    LOG("component_registry: registered subgraph '%s' from %s", type_name.c_str(), path.c_str());
    return true;
}

bool ComponentRegistry::load_plugin(const std::string& path) {
    if (path.size() >= 5 && path.compare(path.size() - 5, 5, ".json") == 0)
        return load_subgraph_json(path);

    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        LOGE("component_registry: dlopen(%s) failed: %s", path.c_str(), dlerror());
        return false;
    }
    auto* fn = reinterpret_cast<DescriptorFn>(dlsym(handle, "eyeballs_node_descriptor"));
    if (!fn) {
        LOGE("component_registry: symbol not found in %s: %s", path.c_str(), dlerror());
        dlclose(handle);
        return false;
    }
    const EyeballsNodeDescriptor* desc = fn();
    if (!desc || desc->version < 1) {
        LOGE("component_registry: ABI version too old in %s", path.c_str());
        dlclose(handle);
        return false;
    }
    entries_[desc->type_name] = RegistryEntry{desc, handle};
    LOG("component_registry: loaded plugin '%s' from %s", desc->type_name, path.c_str());
    return true;
}

const EyeballsNodeDescriptor* ComponentRegistry::find(const std::string& type_name) const {
    auto it = entries_.find(type_name);
    return it != entries_.end() ? it->second.desc : nullptr;
}

std::vector<std::string> ComponentRegistry::type_names() const {
    std::vector<std::string> names;
    names.reserve(entries_.size());
    for (const auto& [name, _] : entries_) names.push_back(name);
    return names;
}

ComponentRegistry::~ComponentRegistry() {
    for (auto& [name, entry] : entries_) {
        if (entry.handle) dlclose(entry.handle);
    }
}
