// Copyright 2025 Travis West
#pragma once
#include "eyeballs_node_abi.h"
#include "param_registry.hpp"
#include "gpu_texture.hpp"
#include <boost/pfr.hpp>
#include <concepts>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>

template<typename N>
concept HasProcess = requires(N& n, double t) { n(t); };

template<typename N>
concept HasSourceHeader = requires { { N::source_header() } -> std::convertible_to<std::string_view>; };

template<typename N>
concept HasSourceCpp = requires { { N::source_cpp() } -> std::convertible_to<std::string_view>; };

template<typename N>
concept HasName = requires { { N::name() } -> std::convertible_to<std::string_view>; };

// Detect whether a field is a texture_output<> by checking for GpuTexture value member.
template<typename F>
concept TextureOutputField = requires(F f) {
    { F::name() } -> std::convertible_to<std::string_view>;
    { f.value } -> std::convertible_to<GpuTexture>;
};

template<typename N>
concept HasOutputs = requires(N& n) { n.outputs; };

// GraphTextureCtx: passed to push_textures so nodes can write textures by port key.
// Defined here so eyeballs_node_abi.hpp consumers can use it without depending on signal_graph.
struct GraphTextureCtx {
    std::string                                   node_id;
    std::unordered_map<std::string, GpuTexture>* textures;  // graph.textures
};

template<HasName Node>
const EyeballsNodeDescriptor* make_descriptor() {
    static void (*process_fn)(void*, double) = nullptr;
    if constexpr (HasProcess<Node>) {
        process_fn = [](void* p, double t) {
            (*static_cast<Node*>(p))(t);
        };
    }

    static void (*push_textures_fn)(void*, void*) = nullptr;
    if constexpr (HasOutputs<Node>) {
        push_textures_fn = [](void* p, void* ctx_ptr) {
            auto* node = static_cast<Node*>(p);
            auto* ctx  = static_cast<GraphTextureCtx*>(ctx_ptr);
            boost::pfr::for_each_field(
                node->outputs,
                [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (TextureOutputField<F>) {
                        std::string key = ctx->node_id + "." + std::string(F::name());
                        (*ctx->textures)[key] = field.value;
                    }
                });
        };
    }

    static const char* source_header_ptr = nullptr;
    if constexpr (HasSourceHeader<Node>) {
        source_header_ptr = Node::source_header().data();
    }
    static const char* source_cpp_ptr = nullptr;
    if constexpr (HasSourceCpp<Node>) {
        source_cpp_ptr = Node::source_cpp().data();
    }

    static EyeballsNodeDescriptor d {
        .version       = EYEBALLS_ABI_VERSION,
        .type_name     = Node::name().data(),
        .description   = nullptr,
        .create        = []() -> void* { return new Node{}; },
        .destroy       = [](void* p) { delete static_cast<Node*>(p); },
        .process       = process_fn,
        .serialize     = [](void* p) -> const char* {
            auto s = to_json(*static_cast<Node*>(p));
            return strdup(s.c_str());
        },
        .free_str      = [](const char* s) { free(const_cast<char*>(s)); },
        .deserialize   = [](void* p, const char* json) {
            from_json(*static_cast<Node*>(p), json);
        },
        .push_textures = push_textures_fn,
        .source_header = source_header_ptr,
        .source_cpp    = source_cpp_ptr,
    };
    return &d;
}

#define EYEBALLS_EXPORT_NODE(Type) \
    extern "C" const EyeballsNodeDescriptor* eyeballs_node_descriptor() { \
        return make_descriptor<Type>(); \
    }
