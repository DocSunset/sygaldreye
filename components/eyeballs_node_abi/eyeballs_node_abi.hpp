// Copyright 2025 Travis West
#pragma once
#include "eyeballs_node_abi.h"
#include "param_registry.hpp"
#include <concepts>
#include <cstdlib>
#include <cstring>
#include <string_view>

template<typename N>
concept HasProcess = requires(N& n, double t) { n(t); };

template<typename N>
concept HasName = requires { { N::name() } -> std::convertible_to<std::string_view>; };

template<HasName Node>
const EyeballsNodeDescriptor* make_descriptor() {
    static void (*process_fn)(void*, double) = nullptr;
    if constexpr (HasProcess<Node>) {
        process_fn = [](void* p, double t) {
            (*static_cast<Node*>(p))(t);
        };
    }

    static EyeballsNodeDescriptor d {
        .version     = EYEBALLS_ABI_VERSION,
        .type_name   = Node::name().data(),
        .description = nullptr,
        .create      = []() -> void* { return new Node{}; },
        .destroy     = [](void* p) { delete static_cast<Node*>(p); },
        .process     = process_fn,
        .serialize   = [](void* p) -> const char* {
            auto s = to_json(*static_cast<Node*>(p));
            return strdup(s.c_str());
        },
        .free_str    = [](const char* s) { free(const_cast<char*>(s)); },
        .deserialize = [](void* p, const char* json) {
            from_json(*static_cast<Node*>(p), json);
        },
    };
    return &d;
}

#define EYEBALLS_EXPORT_NODE(Type) \
    extern "C" const EyeballsNodeDescriptor* eyeballs_node_descriptor() { \
        return make_descriptor<Type>(); \
    }
