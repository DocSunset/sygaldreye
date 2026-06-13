// Copyright 2025 Travis West
#pragma once
#include "eyeballs_node_abi.h"
#include "gpu_texture.hpp"
#include "tri_mesh.hpp"
#include "param_registry.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <boost/pfr.hpp>
#include <concepts>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// ── node-level concepts ──────────────────────────────────────────────────────

template<typename N>
concept HasProcess = requires(N& n, double t) { n(t); };

template<typename N>
concept HasSourceHeader = requires { { N::source_header() } -> std::convertible_to<std::string_view>; };

template<typename N>
concept HasSourceCpp = requires { { N::source_cpp() } -> std::convertible_to<std::string_view>; };

template<typename N>
concept HasName = requires { { N::name() } -> std::convertible_to<std::string_view>; };

// CPU mesh flowing through edges; shared so producers can regenerate while
// consumers hold the previous frame.
using MeshPtr = std::shared_ptr<const TriMeshData>;

// DrawCallCtx: passed to push_draw_calls; accumulates DrawFn callbacks.
struct DrawCallCtx {
    std::string          node_id;  // source node id, set by tick_graph
    std::vector<DrawFn>* calls;    // points into Graph::draw_calls
};

// ── port_schema helpers ──────────────────────────────────────────────────────

namespace detail {

// v6: kind from a shape's value_type (shapes carry no name(), so the old
// PortField-based concepts don't apply).
template<typename T>
constexpr std::string_view v6_kind() {
    if constexpr (std::same_as<T, DrawFn>)             return "draw_call";
    else if constexpr (std::same_as<T, GpuTexture>)    return "texture";
    else if constexpr (std::same_as<T, MeshPtr>)       return "mesh";
    else if constexpr (std::same_as<T, AudioBuffer>)   return "audio";
    else if constexpr (std::same_as<T, Span>)          return "span";
    else if constexpr (std::same_as<T, Eigen::Matrix4f>) return "mat4";
    else if constexpr (std::same_as<T, Eigen::Quaternionf>) return "quat";
    else if constexpr (std::same_as<T, Eigen::Vector4f>) return "vec4";
    else if constexpr (std::same_as<T, Eigen::Vector3f>) return "vec3";
    else if constexpr (std::same_as<T, Eigen::Vector2f>) return "vec2";
    else if constexpr (std::same_as<T, bool>)          return "bool";
    else if constexpr (std::is_arithmetic_v<T>)        return "scalar";
    else if constexpr (std::same_as<T, std::string>)   return "text";
    else                                               return "unknown";
}

// v6: visit a struct's fields with their pfr core names.
template<typename S, typename Fn>
void for_each_named_field(S& s, Fn&& fn) {
    boost::pfr::for_each_field(s,
        [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
            constexpr auto nm = boost::pfr::get_name<I, std::remove_cvref_t<S>>();
            fn(field, std::string_view{nm});
        });
}

} // namespace detail

// ── make_descriptor<T>() ────────────────────────────────────────────────────

template<HasName Node>
    requires HasEndpoints<Node>   // one endpoints struct per node — no legacy shapes
const EyeballsNodeDescriptor* make_descriptor() {
    // process
    static void (*process_fn)(void*, double) = nullptr;
    if constexpr (HasProcess<Node>) {
        process_fn = [](void* p, double t) {
            (*static_cast<Node*>(p))(t);
        };
    }

    // source paths (v3)
    static const char* source_header_ptr = nullptr;
    if constexpr (HasSourceHeader<Node>) {
        source_header_ptr = Node::source_header().data();
    }
    static const char* source_cpp_ptr = nullptr;
    if constexpr (HasSourceCpp<Node>) {
        source_cpp_ptr = Node::source_cpp().data();
    }

    // Descriptor surface — populated by the endpoints branch below.
    static const char* port_schema_ptr = nullptr;
    static void (*push_outputs_fn)(void*, EyeballsOutputCtx*) = nullptr;
    static void (*push_draw_calls_fn)(void*, void*) = nullptr;
    static void (*set_scalar_in_fn)(void*, const char*, double) = nullptr;
    static void (*set_vec2_in_fn)(void*, const char*, float, float) = nullptr;
    static void (*set_vec3_in_fn)(void*, const char*, float, float, float) = nullptr;
    static void (*set_vec4_in_fn)(void*, const char*, float, float, float, float) = nullptr;
    static void (*set_mat4_in_fn)(void*, const char*, const float*) = nullptr;
    static void (*set_quat_in_fn)(void*, const char*, float, float, float, float) = nullptr;
    static void (*set_texture_in_fn)(void*, const char*, unsigned int, int, int,
                                     unsigned int, unsigned int) = nullptr;
    static void (*set_audio_in_fn)(void*, const char*, const float*, int, int, int) = nullptr;
    static void (*set_mesh_in_fn)(void*, const char*, const void*) = nullptr;
    static void (*set_text_in_fn)(void*, const char*, const char*) = nullptr;
    static void (*set_drawfn_in_fn)(void*, const char*, const void*) = nullptr;

    // ── endpoints v6: one struct, shape-directed ────────────────────────────
    static int (*connect_fn)(void*, const char*, const void*) = nullptr;
    static const void* (*output_ptr_fn)(void*, const char*)   = nullptr;
    if constexpr (HasEndpoints<Node>) {
        static std::string v6_schema = [] {
            std::string ins = "[", outs = "[";
            Node tmp{};
            detail::for_each_named_field(tmp.endpoints,
                [&](auto& f, std::string_view nm) {
                    using F = std::remove_cvref_t<decltype(f)>;
                    auto add = [&](std::string& s, std::string_view kind,
                                   const std::string& extra) {
                        if (s.size() > 1) s += ',';
                        s += "{\"name\":\""; s += nm;
                        s += "\",\"kind\":\""; s += kind; s += '"';
                        s += extra; s += '}';
                    };
                    if constexpr (V6EventIn<F>)       add(ins,  "bang", "");
                    else if constexpr (V6EventOut<F>) add(outs, "bang", "");
                    else if constexpr (V6Normalled<F>) {
                        std::string extra;
                        if constexpr (std::is_arithmetic_v<typename F::value_type>) {
                            char buf[64];
                            std::snprintf(buf, sizeof(buf), ",\"min\":%g,\"max\":%g",
                                          double(F::min()), double(F::max()));
                            extra = buf;
                        }
                        add(ins, detail::v6_kind<typename F::value_type>(), extra);
                    }
                    else if constexpr (V6Input<F>)
                        add(ins, detail::v6_kind<typename F::value_type>(), "");
                    else if constexpr (V6Output<F>)
                        add(outs, detail::v6_kind<typename F::value_type>(), "");
                });
            return "{\"inputs\":" + ins + "],\"outputs\":" + outs + "]}";
        }();
        port_schema_ptr = v6_schema.c_str();

        push_outputs_fn = [](void* p, EyeballsOutputCtx* ctx) {
            auto* node = static_cast<Node*>(p);
            detail::for_each_named_field(node->endpoints,
                [&](auto& f, std::string_view nm) {
                    using F = std::remove_cvref_t<decltype(f)>;
                    if constexpr (V6EventOut<F>) {
                        ctx->emit_scalar(ctx->store, ctx->node_id, nm.data(),
                                         f.triggered ? 1.0 : 0.0);
                    } else if constexpr (V6Output<F>) {
                        using T = typename F::value_type;
                        if constexpr (std::same_as<T, DrawFn>) {
                            if (ctx->emit_drawfn && f.value)
                                ctx->emit_drawfn(ctx->store, ctx->node_id, nm.data(),
                                                 static_cast<const void*>(&f.value));
                        } else if constexpr (std::same_as<T, MeshPtr>) {
                            if (ctx->emit_mesh && f.value)
                                ctx->emit_mesh(ctx->store, ctx->node_id, nm.data(),
                                               static_cast<const void*>(&f.value));
                        } else if constexpr (std::same_as<T, GpuTexture>) {
                            ctx->emit_texture(ctx->store, ctx->node_id, nm.data(),
                                              f.value.id, f.value.width, f.value.height,
                                              f.value.internal_format, f.value.filter);
                        } else if constexpr (std::same_as<T, AudioBuffer>) {
                            ctx->emit_audio(ctx->store, ctx->node_id, nm.data(),
                                            f.value.data, f.value.frames,
                                            f.value.channels, f.value.sample_rate);
                        } else if constexpr (std::same_as<T, Eigen::Matrix4f>) {
                            ctx->emit_mat4(ctx->store, ctx->node_id, nm.data(),
                                           f.value.data());
                        } else if constexpr (std::same_as<T, Eigen::Quaternionf>) {
                            ctx->emit_quat(ctx->store, ctx->node_id, nm.data(),
                                           f.value.x(), f.value.y(), f.value.z(), f.value.w());
                        } else if constexpr (std::same_as<T, Eigen::Vector4f>) {
                            ctx->emit_vec4(ctx->store, ctx->node_id, nm.data(),
                                           f.value.x(), f.value.y(), f.value.z(), f.value.w());
                        } else if constexpr (std::same_as<T, Eigen::Vector3f>) {
                            ctx->emit_vec3(ctx->store, ctx->node_id, nm.data(),
                                           f.value.x(), f.value.y(), f.value.z());
                        } else if constexpr (std::same_as<T, Eigen::Vector2f>) {
                            ctx->emit_vec2(ctx->store, ctx->node_id, nm.data(),
                                           f.value.x(), f.value.y());
                        } else if constexpr (std::same_as<T, Span>) {
                            if (ctx->emit_span)
                                ctx->emit_span(ctx->store, ctx->node_id, nm.data(),
                                               f.value.data, f.value.rows,
                                               f.value.cols);
                        } else if constexpr (std::same_as<T, std::string>) {
                            if (ctx->emit_text)
                                ctx->emit_text(ctx->store, ctx->node_id, nm.data(),
                                               f.value.c_str());
                        } else if constexpr (std::is_arithmetic_v<T>) {
                            ctx->emit_scalar(ctx->store, ctx->node_id, nm.data(),
                                             static_cast<double>(f.value));
                        }
                    }
                });
        };

        push_draw_calls_fn = [](void* p, void* ctx_ptr) {
            auto* node = static_cast<Node*>(p);
            auto* ctx  = static_cast<DrawCallCtx*>(ctx_ptr);
            detail::for_each_named_field(node->endpoints,
                [&](auto& f, std::string_view) {
                    using F = std::remove_cvref_t<decltype(f)>;
                    if constexpr (V6Output<F>) {
                        if constexpr (std::same_as<typename F::value_type, DrawFn>)
                            if (f.value) ctx->calls->push_back(f.value);
                    }
                });
        };

        // Param writers: normalled fallback / cv offset / event trigger.
        // (in<T> has no storage by design — connection-only.)
        set_scalar_in_fn = [](void* p, const char* port, double v) {
            auto* node = static_cast<Node*>(p);
            detail::for_each_named_field(node->endpoints,
                [&](auto& f, std::string_view nm) {
                    using F = std::remove_cvref_t<decltype(f)>;
                    if (nm != std::string_view(port)) return;
                    if constexpr (V6EventIn<F>) f.triggered = (v != 0.0);
                    else if constexpr (V6Normalled<F>) {
                        if constexpr (std::is_arithmetic_v<typename F::value_type>)
                            f.fallback = static_cast<typename F::value_type>(v);
                    } else if constexpr (V6Cv<F>) {
                        if constexpr (std::is_arithmetic_v<typename F::value_type>)
                            f.offset = static_cast<typename F::value_type>(v);
                    }
                });
        };
        set_vec3_in_fn = [](void* p, const char* port, float x, float y, float z) {
            auto* node = static_cast<Node*>(p);
            detail::for_each_named_field(node->endpoints,
                [&](auto& f, std::string_view nm) {
                    using F = std::remove_cvref_t<decltype(f)>;
                    if (nm != std::string_view(port)) return;
                    if constexpr (V6Normalled<F>) {
                        if constexpr (std::same_as<typename F::value_type, Eigen::Vector3f>)
                            f.fallback = Eigen::Vector3f{x, y, z};
                    } else if constexpr (V6Cv<F>) {
                        if constexpr (std::same_as<typename F::value_type, Eigen::Vector3f>)
                            f.offset = Eigen::Vector3f{x, y, z};
                    }
                });
        };
        set_quat_in_fn = [](void* p, const char* port, float x, float y, float z, float w) {
            auto* node = static_cast<Node*>(p);
            detail::for_each_named_field(node->endpoints,
                [&](auto& f, std::string_view nm) {
                    using F = std::remove_cvref_t<decltype(f)>;
                    if (nm != std::string_view(port)) return;
                    if constexpr (V6Normalled<F>) {
                        if constexpr (std::same_as<typename F::value_type, Eigen::Quaternionf>)
                            f.fallback = Eigen::Quaternionf{w, x, y, z};
                    } else if constexpr (V6Cv<F>) {
                        if constexpr (std::same_as<typename F::value_type, Eigen::Quaternionf>)
                            f.offset = Eigen::Quaternionf{w, x, y, z};
                    }
                });
        };

        set_text_in_fn = [](void* p, const char* port, const char* utf8) {
            auto* node = static_cast<Node*>(p);
            detail::for_each_named_field(node->endpoints,
                [&](auto& f, std::string_view nm) {
                    using F = std::remove_cvref_t<decltype(f)>;
                    if (nm != std::string_view(port)) return;
                    if constexpr (V6Normalled<F>) {
                        if constexpr (std::same_as<typename F::value_type, std::string>)
                            f.fallback = utf8;
                    }
                });
        };

        connect_fn = [](void* p, const char* port, const void* src) -> int {
            auto* node = static_cast<Node*>(p);
            int hit = 0;
            detail::for_each_named_field(node->endpoints,
                [&](auto& f, std::string_view nm) {
                    using F = std::remove_cvref_t<decltype(f)>;
                    if constexpr (V6Input<F>) {
                        if (nm == std::string_view(port)) {
                            f.src = static_cast<const typename F::value_type*>(src);
                            hit = 1;
                        }
                    }
                });
            return hit;
        };
        output_ptr_fn = [](void* p, const char* port) -> const void* {
            auto* node = static_cast<Node*>(p);
            const void* out = nullptr;
            detail::for_each_named_field(node->endpoints,
                [&](auto& f, std::string_view nm) {
                    using F = std::remove_cvref_t<decltype(f)>;
                    if constexpr (V6Output<F>) {
                        if (nm == std::string_view(port)) out = &f.value;
                    } else if constexpr (V6EventOut<F>) {
                        // events expose their flag: readers see exactly the
                        // tick the producer fired (recomputed per tick)
                        if (nm == std::string_view(port)) out = &f.triggered;
                    }
                });
            return out;
        };
    }

    static EyeballsNodeDescriptor d {
        .version          = EYEBALLS_ABI_VERSION,
        .type_name        = Node::name().data(),
        .description      = nullptr,
        .create           = []() -> void* { return new Node{}; },
        .destroy          = [](void* p) { delete static_cast<Node*>(p); },
        .process          = process_fn,
        .serialize        = [](void* p) -> const char* {
            auto s = to_json(*static_cast<Node*>(p));
            return strdup(s.c_str());
        },
        .free_str         = [](const char* s) { free(const_cast<char*>(s)); },
        .deserialize      = [](void* p, const char* json) {
            from_json(*static_cast<Node*>(p), json);
        },
        .push_textures    = nullptr,   // legacy v2 — superseded by push_outputs
        .source_header    = source_header_ptr,
        .source_cpp       = source_cpp_ptr,
        .port_schema      = port_schema_ptr,
        .push_outputs     = push_outputs_fn,
        .push_draw_calls  = push_draw_calls_fn,
        .set_scalar_in    = set_scalar_in_fn,
        .set_vec2_in      = set_vec2_in_fn,
        .set_vec3_in      = set_vec3_in_fn,
        .set_vec4_in      = set_vec4_in_fn,
        .set_mat4_in      = set_mat4_in_fn,
        .set_quat_in      = set_quat_in_fn,
        .set_texture_in   = set_texture_in_fn,
        .set_audio_in     = set_audio_in_fn,
        .set_drawfn_in    = set_drawfn_in_fn,
        .set_mesh_in      = set_mesh_in_fn,
        .connect          = connect_fn,
        .output_ptr       = output_ptr_fn,
        .set_text_in      = set_text_in_fn,
    };
    return &d;
}

#define EYEBALLS_EXPORT_NODE(Type) \
    extern "C" const EyeballsNodeDescriptor* eyeballs_node_descriptor() { \
        return make_descriptor<Type>(); \
    }
