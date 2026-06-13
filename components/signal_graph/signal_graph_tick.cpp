// Copyright 2025 Travis West
#include "signal_graph.hpp"
#include "signal_graph_plan.hpp"
#include <cstring>
#include <optional>

// ── shared executor primitives (every scheduler uses these) ────────────────

void apply_value(const NodeInstance& n, const char* port, const PortValue& value) {
    std::visit([&](const auto& val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, double>) {
            if (n.desc->set_scalar_in)
                n.desc->set_scalar_in(n.data, port, val);
        } else if constexpr (std::is_same_v<T, Eigen::Vector2f>) {
            if (n.desc->set_vec2_in)
                n.desc->set_vec2_in(n.data, port, val.x(), val.y());
        } else if constexpr (std::is_same_v<T, Eigen::Vector3f>) {
            if (n.desc->set_vec3_in)
                n.desc->set_vec3_in(n.data, port, val.x(), val.y(), val.z());
        } else if constexpr (std::is_same_v<T, Eigen::Vector4f>) {
            if (n.desc->set_vec4_in)
                n.desc->set_vec4_in(n.data, port, val.x(), val.y(), val.z(), val.w());
        } else if constexpr (std::is_same_v<T, Eigen::Matrix4f>) {
            if (n.desc->set_mat4_in)
                n.desc->set_mat4_in(n.data, port, val.data());
        } else if constexpr (std::is_same_v<T, Eigen::Quaternionf>) {
            if (n.desc->set_quat_in)
                n.desc->set_quat_in(n.data, port, val.x(), val.y(), val.z(), val.w());
        } else if constexpr (std::is_same_v<T, GpuTexture>) {
            if (n.desc->set_texture_in)
                n.desc->set_texture_in(n.data, port, val.id, val.width, val.height,
                                       val.internal_format, val.filter);
        } else if constexpr (std::is_same_v<T, AudioBuffer>) {
            if (n.desc->set_audio_in)
                n.desc->set_audio_in(n.data, port, val.data, val.frames,
                                     val.channels, val.sample_rate);
        } else if constexpr (std::is_same_v<T, DrawFn>) {
            if (n.desc->set_drawfn_in)
                n.desc->set_drawfn_in(n.data, port, static_cast<const void*>(&val));
        } else if constexpr (std::is_same_v<T, MeshPtr>) {
            if (n.desc->set_mesh_in)
                n.desc->set_mesh_in(n.data, port, static_cast<const void*>(&val));
        } else if constexpr (std::is_same_v<T, std::string>) {
            if (n.desc->version >= 6 && n.desc->set_text_in)
                n.desc->set_text_in(n.data, port, val.c_str());
        }
    }, value);
}

EyeballsOutputCtx output_ctx(std::unordered_map<std::string, PortValue>* store,
                             const char* node_id) {
    using Store = std::unordered_map<std::string, PortValue>;
    EyeballsOutputCtx ctx{};
    ctx.store   = store;
    ctx.node_id = node_id;
    ctx.emit_scalar = [](void* s, const char* nid, const char* port, double v) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] = v;
    };
    ctx.emit_vec2 = [](void* s, const char* nid, const char* port, float x, float y) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] = Eigen::Vector2f{x, y};
    };
    ctx.emit_vec3 = [](void* s, const char* nid, const char* port,
                       float x, float y, float z) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] = Eigen::Vector3f{x, y, z};
    };
    ctx.emit_vec4 = [](void* s, const char* nid, const char* port,
                       float x, float y, float z, float w) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] = Eigen::Vector4f{x, y, z, w};
    };
    ctx.emit_mat4 = [](void* s, const char* nid, const char* port, const float* c16) {
        Eigen::Matrix4f mat;
        std::copy(c16, c16 + 16, mat.data());
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] = mat;
    };
    ctx.emit_quat = [](void* s, const char* nid, const char* port,
                       float x, float y, float z, float w) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] =
            Eigen::Quaternionf{w, x, y, z};
    };
    ctx.emit_texture = [](void* s, const char* nid, const char* port,
                          unsigned int id, int w, int h,
                          unsigned int fmt, unsigned int filt) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] =
            GpuTexture{id, w, h, fmt, filt};
    };
    ctx.emit_drawfn = [](void* s, const char* nid, const char* port, const void* fn) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] =
            *static_cast<const DrawFn*>(fn);
    };
    ctx.emit_mesh = [](void* s, const char* nid, const char* port, const void* mesh) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] =
            *static_cast<const MeshPtr*>(mesh);
    };
    ctx.emit_audio = [](void* s, const char* nid, const char* port,
                        const float* data, int frames, int channels, int rate) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] =
            AudioBuffer{data, frames, channels, rate};
    };
    ctx.emit_text = [](void* s, const char* nid, const char* port, const char* utf8) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] =
            std::string{utf8};
    };
    ctx.emit_span = [](void* s, const char* nid, const char* port,
                       const float* data, int rows, int cols) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] =
            Span{data, rows, cols};
    };
    return ctx;
}

// Typed read of a producer's owned storage — the by-ref edge primitive.
// Kinds mirror detail::v6_kind; scalar outs are float by convention.
std::optional<PortValue> read_output(const NodeInstance& n,
                                     const std::string& port,
                                     const std::string& kind) {
    if (!n.desc->output_ptr) return std::nullopt;
    const void* p = n.desc->output_ptr(n.data, port.c_str());
    if (!p) return std::nullopt;
    if (kind == "scalar")   return PortValue{double(*static_cast<const float*>(p))};
    if (kind == "bang")     return PortValue{*static_cast<const bool*>(p) ? 1.0 : 0.0};
    if (kind == "bool")     return PortValue{*static_cast<const bool*>(p) ? 1.0 : 0.0};
    if (kind == "vec2")     return PortValue{*static_cast<const Eigen::Vector2f*>(p)};
    if (kind == "vec3")     return PortValue{*static_cast<const Eigen::Vector3f*>(p)};
    if (kind == "vec4")     return PortValue{*static_cast<const Eigen::Vector4f*>(p)};
    if (kind == "quat")     return PortValue{*static_cast<const Eigen::Quaternionf*>(p)};
    if (kind == "mat4")     return PortValue{*static_cast<const Eigen::Matrix4f*>(p)};
    if (kind == "texture")  return PortValue{*static_cast<const GpuTexture*>(p)};
    if (kind == "audio")    return PortValue{*static_cast<const AudioBuffer*>(p)};
    if (kind == "mesh")     return PortValue{*static_cast<const MeshPtr*>(p)};
    if (kind == "draw_call")return PortValue{*static_cast<const DrawFn*>(p)};
    if (kind == "text")     return PortValue{*static_cast<const std::string*>(p)};
    if (kind == "span")     return PortValue{*static_cast<const Span*>(p)};
    return std::nullopt;
}

std::optional<PortValue> read_output(const EdgeApplier& a) {
    if (!a.from) return std::nullopt;
    return read_output(*a.from, a.edge->from_port, a.kind);
}

// ── the render (frame) scheduler ────────────────────────────────────────────

void tick_graph(Graph& g, double time_s) {
    g.draw_calls.clear();
    if (!g.plan) { g.plan = build_plan(g); wire_plan(g); }
    TickPlan& plan = *g.plan;

    for (std::size_t idx : plan.order) {
        auto& n = g.nodes[idx];

        for (auto& a : plan.appliers[idx])
            if (auto src = read_output(a))
                apply_value(n, a.edge->to_port.c_str(), *src);
        for (auto* d : plan.delayed[idx])
            if (d->prev)
                apply_value(n, d->applier.edge->to_port.c_str(), *d->prev);

        if (n.desc->process) n.desc->process(n.data, time_s);

        // A node whose draw output feeds an edge is consumed by that
        // consumer (render_target etc.) and skips the global pass.
        if (n.desc->push_draw_calls && !plan.draw_consumed[idx]) {
            DrawCallCtx ctx{n.id, &g.draw_calls};
            n.desc->push_draw_calls(n.data, &ctx);
        }
    }

    // z⁻¹ capture for this region's delays (the block scheduler captures
    // its own at block end).
    for (auto& d : plan.delays)
        if (d.region == port_types::Rate::Frame)
            if (auto src = read_output(d.applier))
                d.prev = *src;
}

// Pull observability: one frame-coherent sweep of every node's outputs.
// Execution never builds this — it exists only when someone asks.
std::unordered_map<std::string, PortValue> snapshot_values(const Graph& g) {
    std::unordered_map<std::string, PortValue> out;
    for (const auto& n : g.nodes)
        if (n.desc->push_outputs) {
            EyeballsOutputCtx ctx = output_ctx(&out, n.id.c_str());
            n.desc->push_outputs(n.data, &ctx);
        }
    return out;
}
