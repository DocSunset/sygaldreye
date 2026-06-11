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

template<typename N>
concept HasOutputs = requires(N& n) { n.outputs; };

template<typename N>
concept HasInputs = requires(N& n) { n.inputs; };

// ── port-field concepts ──────────────────────────────────────────────────────

// Any field with name() and a .value member that declares value_type.
template<typename F>
concept PortField = requires(F f) {
    { F::name() } -> std::convertible_to<std::string_view>;
    typename F::value_type;
    { f.value };
};

template<typename F>
concept DrawCallField = PortField<F> &&
    std::same_as<typename F::value_type, DrawFn>;

template<typename F>
concept ScalarPortField = PortField<F> &&
    (std::same_as<typename F::value_type, float> ||
     std::same_as<typename F::value_type, double>);

template<typename F>
concept Vec2PortField = PortField<F> &&
    std::same_as<typename F::value_type, Eigen::Vector2f>;

template<typename F>
concept Vec3PortField = PortField<F> &&
    std::same_as<typename F::value_type, Eigen::Vector3f>;

template<typename F>
concept Vec4PortField = PortField<F> &&
    std::same_as<typename F::value_type, Eigen::Vector4f>;

template<typename F>
concept Mat4PortField = PortField<F> &&
    std::same_as<typename F::value_type, Eigen::Matrix4f>;

template<typename F>
concept QuatPortField = PortField<F> &&
    std::same_as<typename F::value_type, Eigen::Quaternionf>;

template<typename F>
concept TexturePortField = PortField<F> &&
    std::same_as<typename F::value_type, GpuTexture>;

template<typename F>
concept AudioPortField = PortField<F> &&
    std::same_as<typename F::value_type, AudioBuffer>;

// CPU mesh flowing through edges; shared so producers can regenerate while
// consumers hold the previous frame.
using MeshPtr = std::shared_ptr<const TriMeshData>;

template<typename F>
concept MeshPortField = PortField<F> &&
    std::same_as<typename F::value_type, MeshPtr>;

// SliderField and ToggleField are defined in param_registry.hpp (already included).

// GraphTextureCtx: legacy v2 context for push_textures. Kept for ABI compatibility.
// New code should use EyeballsOutputCtx (ABI v4) instead.
struct GraphTextureCtx {
    std::string node_id;
    // textures map removed — superseded by Graph::values (PortValue variant map).
};

// DrawCallCtx: passed to push_draw_calls; accumulates DrawFn callbacks.
struct DrawCallCtx {
    std::string          node_id;  // source node id, set by tick_graph
    std::vector<DrawFn>* calls;    // points into Graph::draw_calls
};

// ── port_schema helpers ──────────────────────────────────────────────────────

namespace detail {

template<typename F>
constexpr std::string_view port_kind() {
    if constexpr (DrawCallField<F>)   return "draw_call";
    else if constexpr (TexturePortField<F>) return "texture";
    else if constexpr (MeshPortField<F>)   return "mesh";
    else if constexpr (AudioPortField<F>)  return "audio";
    else if constexpr (Mat4PortField<F>)   return "mat4";
    else if constexpr (QuatPortField<F>)   return "quat";
    else if constexpr (Vec4PortField<F>)   return "vec4";
    else if constexpr (Vec3PortField<F>)   return "vec3";
    else if constexpr (Vec2PortField<F>)   return "vec2";
    else if constexpr (ScalarPortField<F>) return "scalar";
    else if constexpr (SliderField<F>)     return "scalar";
    else if constexpr (ToggleField<F>)     return "bool";
    else if constexpr (TextField<F>)       return "text";
    else if constexpr (BangField<F>)       return "bang";
    else                                   return "unknown";
}

// Append port JSON entries for a struct S (inputs or outputs) into `out`.
template<typename S>
void append_ports_json(const S& s, std::string& out) {
    bool first = true;
    boost::pfr::for_each_field(s,
        [&]<std::size_t I>(const auto& field, std::integral_constant<std::size_t, I>) {
            using F = std::remove_cvref_t<decltype(field)>;
            constexpr auto nm   = F::name();
            constexpr auto kind = port_kind<F>();
            if (!first) out += ',';
            first = false;
            out += "{\"name\":\"";
            out += nm;
            out += "\",\"kind\":\"";
            out += kind;
            out += '"';
            if constexpr (SliderField<F>) {  // range so editors scale correctly
                char buf[64];
                std::snprintf(buf, sizeof(buf), ",\"min\":%g,\"max\":%g",
                              double(F::min()), double(F::max()));
                out += buf;
            }
            out += '}';
        });
}

} // namespace detail

// ── make_descriptor<T>() ────────────────────────────────────────────────────

template<HasName Node>
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

    // port_schema (v4): built once, stored as a static string
    static const char* port_schema_ptr = nullptr;
    if constexpr (HasInputs<Node> || HasOutputs<Node>) {
        static std::string schema_str = []() -> std::string {
            std::string s = "{";
            if constexpr (HasInputs<Node>) {
                s += "\"inputs\":[";
                Node tmp{};
                detail::append_ports_json(tmp.inputs, s);
                s += ']';
            } else {
                s += "\"inputs\":[]";
            }
            s += ',';
            if constexpr (HasOutputs<Node>) {
                s += "\"outputs\":[";
                Node tmp{};
                detail::append_ports_json(tmp.outputs, s);
                s += ']';
            } else {
                s += "\"outputs\":[]";
            }
            s += '}';
            return s;
        }();
        port_schema_ptr = schema_str.c_str();
    }

    // push_outputs (v4)
    static void (*push_outputs_fn)(void*, EyeballsOutputCtx*) = nullptr;
    if constexpr (HasOutputs<Node>) {
        push_outputs_fn = [](void* p, EyeballsOutputCtx* ctx) {
            auto* node = static_cast<Node*>(p);
            boost::pfr::for_each_field(node->outputs,
                [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                    using F = std::remove_cvref_t<decltype(field)>;
                    constexpr auto nm = F::name();
                    if constexpr (DrawCallField<F>) {
                        // also published as a value so draw-call EDGES work;
                        // tick_graph skips the global pass for consumed nodes
                        if (ctx->emit_drawfn && field.value)
                            ctx->emit_drawfn(ctx->store, ctx->node_id, nm.data(),
                                             static_cast<const void*>(&field.value));
                    } else if constexpr (MeshPortField<F>) {
                        if (ctx->emit_mesh && field.value)
                            ctx->emit_mesh(ctx->store, ctx->node_id, nm.data(),
                                           static_cast<const void*>(&field.value));
                    } else if constexpr (ScalarPortField<F>) {
                        ctx->emit_scalar(ctx->store, ctx->node_id, nm.data(),
                                         static_cast<double>(field.value));
                    } else if constexpr (Vec2PortField<F>) {
                        ctx->emit_vec2(ctx->store, ctx->node_id, nm.data(),
                                       field.value.x(), field.value.y());
                    } else if constexpr (Vec3PortField<F>) {
                        ctx->emit_vec3(ctx->store, ctx->node_id, nm.data(),
                                       field.value.x(), field.value.y(), field.value.z());
                    } else if constexpr (Vec4PortField<F>) {
                        ctx->emit_vec4(ctx->store, ctx->node_id, nm.data(),
                                       field.value.x(), field.value.y(),
                                       field.value.z(), field.value.w());
                    } else if constexpr (Mat4PortField<F>) {
                        ctx->emit_mat4(ctx->store, ctx->node_id, nm.data(),
                                       field.value.data());
                    } else if constexpr (QuatPortField<F>) {
                        ctx->emit_quat(ctx->store, ctx->node_id, nm.data(),
                                       field.value.x(), field.value.y(),
                                       field.value.z(), field.value.w());
                    } else if constexpr (TexturePortField<F>) {
                        ctx->emit_texture(ctx->store, ctx->node_id, nm.data(),
                                          field.value.id, field.value.width,
                                          field.value.height,
                                          field.value.internal_format,
                                          field.value.filter);
                    } else if constexpr (AudioPortField<F>) {
                        ctx->emit_audio(ctx->store, ctx->node_id, nm.data(),
                                        field.value.data, field.value.frames,
                                        field.value.channels, field.value.sample_rate);
                    } else if constexpr (BangField<F>) {
                        // Event rate as a 0/1 copy: true for exactly the
                        // tick the producer fires (producer resets next tick).
                        ctx->emit_scalar(ctx->store, ctx->node_id, nm.data(),
                                         field.triggered ? 1.0 : 0.0);
                    }
                });
        };
    }

    // push_draw_calls (v4)
    static void (*push_draw_calls_fn)(void*, void*) = nullptr;
    if constexpr (HasOutputs<Node>) {
        push_draw_calls_fn = [](void* p, void* ctx_ptr) {
            auto* node = static_cast<Node*>(p);
            auto* ctx  = static_cast<DrawCallCtx*>(ctx_ptr);
            boost::pfr::for_each_field(node->outputs,
                [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (DrawCallField<F>) {
                        if (field.value) ctx->calls->push_back(field.value);
                    }
                });
        };
    }

    // set_scalar_in (v4)
    static void (*set_scalar_in_fn)(void*, const char*, double) = nullptr;
    if constexpr (HasInputs<Node>) {
        set_scalar_in_fn = [](void* p, const char* port_name, double v) {
            auto* node = static_cast<Node*>(p);
            boost::pfr::for_each_field(node->inputs,
                [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (ScalarPortField<F> || SliderField<F>) {
                        if (F::name() == std::string_view(port_name))
                            field.value = static_cast<typename std::remove_cvref_t<decltype(field.value)>>(v);
                    } else if constexpr (ToggleField<F>) {
                        if (F::name() == std::string_view(port_name))
                            field.value = (v != 0.0);
                    } else if constexpr (BangField<F>) {
                        if (F::name() == std::string_view(port_name))
                            field.triggered = (v != 0.0);
                    }
                });
        };
    }

    // set_vec2_in (v4)
    static void (*set_vec2_in_fn)(void*, const char*, float, float) = nullptr;
    if constexpr (HasInputs<Node>) {
        set_vec2_in_fn = [](void* p, const char* port_name, float x, float y) {
            auto* node = static_cast<Node*>(p);
            boost::pfr::for_each_field(node->inputs,
                [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (Vec2PortField<F>) {
                        if (F::name() == std::string_view(port_name))
                            field.value = Eigen::Vector2f{x, y};
                    }
                });
        };
    }

    // set_vec3_in (v4)
    static void (*set_vec3_in_fn)(void*, const char*, float, float, float) = nullptr;
    if constexpr (HasInputs<Node>) {
        set_vec3_in_fn = [](void* p, const char* port_name, float x, float y, float z) {
            auto* node = static_cast<Node*>(p);
            boost::pfr::for_each_field(node->inputs,
                [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (Vec3PortField<F>) {
                        if (F::name() == std::string_view(port_name))
                            field.value = Eigen::Vector3f{x, y, z};
                    }
                });
        };
    }

    // set_vec4_in (v4)
    static void (*set_vec4_in_fn)(void*, const char*, float, float, float, float) = nullptr;
    if constexpr (HasInputs<Node>) {
        set_vec4_in_fn = [](void* p, const char* port_name, float x, float y, float z, float w) {
            auto* node = static_cast<Node*>(p);
            boost::pfr::for_each_field(node->inputs,
                [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (Vec4PortField<F>) {
                        if (F::name() == std::string_view(port_name))
                            field.value = Eigen::Vector4f{x, y, z, w};
                    }
                });
        };
    }

    // set_mat4_in (v4)
    static void (*set_mat4_in_fn)(void*, const char*, const float*) = nullptr;
    if constexpr (HasInputs<Node>) {
        set_mat4_in_fn = [](void* p, const char* port_name, const float* col16) {
            auto* node = static_cast<Node*>(p);
            boost::pfr::for_each_field(node->inputs,
                [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (Mat4PortField<F>) {
                        if (F::name() == std::string_view(port_name))
                            field.value = Eigen::Map<const Eigen::Matrix4f>(col16);
                    }
                });
        };
    }

    // set_quat_in (v4)
    static void (*set_quat_in_fn)(void*, const char*, float, float, float, float) = nullptr;
    if constexpr (HasInputs<Node>) {
        set_quat_in_fn = [](void* p, const char* port_name, float x, float y, float z, float w) {
            auto* node = static_cast<Node*>(p);
            boost::pfr::for_each_field(node->inputs,
                [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (QuatPortField<F>) {
                        if (F::name() == std::string_view(port_name))
                            field.value = Eigen::Quaternionf{w, x, y, z};
                    }
                });
        };
    }

    // set_texture_in (v4)
    static void (*set_texture_in_fn)(void*, const char*, unsigned int, int, int,
                                     unsigned int, unsigned int) = nullptr;
    if constexpr (HasInputs<Node>) {
        set_texture_in_fn = [](void* p, const char* port_name,
                               unsigned int gl_id, int w, int h,
                               unsigned int fmt, unsigned int filter) {
            auto* node = static_cast<Node*>(p);
            boost::pfr::for_each_field(node->inputs,
                [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (TexturePortField<F>) {
                        if (F::name() == std::string_view(port_name))
                            field.value = GpuTexture{gl_id, w, h, fmt, filter};
                    }
                });
        };
    }

    // set_audio_in (v4)
    static void (*set_audio_in_fn)(void*, const char*, const float*, int, int, int) = nullptr;
    if constexpr (HasInputs<Node>) {
        set_audio_in_fn = [](void* p, const char* port_name,
                             const float* samples, int frames, int channels, int rate) {
            auto* node = static_cast<Node*>(p);
            boost::pfr::for_each_field(node->inputs,
                [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (AudioPortField<F>) {
                        if (F::name() == std::string_view(port_name))
                            field.value = AudioBuffer{samples, frames, channels, rate};
                    }
                });
        };
    }

    // set_mesh_in (v5)
    static void (*set_mesh_in_fn)(void*, const char*, const void*) = nullptr;
    if constexpr (HasInputs<Node>) {
        set_mesh_in_fn = [](void* p, const char* port_name, const void* mesh) {
            auto* node = static_cast<Node*>(p);
            boost::pfr::for_each_field(node->inputs,
                [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (MeshPortField<F>) {
                        if (F::name() == std::string_view(port_name))
                            field.value = *static_cast<const MeshPtr*>(mesh);
                    }
                });
        };
    }

    // set_drawfn_in (v5)
    static void (*set_drawfn_in_fn)(void*, const char*, const void*) = nullptr;
    if constexpr (HasInputs<Node>) {
        set_drawfn_in_fn = [](void* p, const char* port_name, const void* fn) {
            auto* node = static_cast<Node*>(p);
            boost::pfr::for_each_field(node->inputs,
                [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (DrawCallField<F>) {
                        if (F::name() == std::string_view(port_name))
                            field.value = *static_cast<const DrawFn*>(fn);
                    }
                });
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
    };
    return &d;
}

#define EYEBALLS_EXPORT_NODE(Type) \
    extern "C" const EyeballsNodeDescriptor* eyeballs_node_descriptor() { \
        return make_descriptor<Type>(); \
    }
