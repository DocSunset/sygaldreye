// Copyright 2025 Travis West
#include "component_registry.hpp"
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

bool ComponentRegistry::load_plugin(const std::string& path) {
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
