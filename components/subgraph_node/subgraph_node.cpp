// Copyright 2025 Travis West
#include "subgraph_node.hpp"
#include "signal_graph_plan.hpp"
#include <algorithm>

// ── SubgraphNode ─────────────────────────────────────────────────────────────

SubgraphNode::SubgraphNode(std::unique_ptr<Graph> inner)
    : inner_(std::move(inner)) {}

void SubgraphNode::cache_inlet(const std::string& name, PortValue val) {
    inlet_cache_[name] = std::move(val);
}

void SubgraphNode::operator()(double t) {
    for (const auto& decl : inner_->inlets) {
        auto cv = inlet_cache_.find(decl.name);
        if (cv == inlet_cache_.end()) continue;
        auto it = std::find_if(inner_->nodes.begin(), inner_->nodes.end(),
                               [&](const NodeInstance& n) { return n.id == decl.node; });
        if (it == inner_->nodes.end()) continue;
        apply_value(*it, decl.port.c_str(), cv->second);
    }
    tick_graph(*inner_, t);
}

void SubgraphNode::push_outlets(EyeballsOutputCtx* ctx) const {
    for (const auto& decl : inner_->outlets) {
        auto it = inner_->values.find(decl.node + "." + decl.port);
        if (it == inner_->values.end()) continue;
        const char* nm = decl.name.c_str();
        std::visit([&](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, double>) {
                ctx->emit_scalar(ctx->store, ctx->node_id, nm, val);
            } else if constexpr (std::is_same_v<T, Eigen::Vector2f>) {
                ctx->emit_vec2(ctx->store, ctx->node_id, nm, val.x(), val.y());
            } else if constexpr (std::is_same_v<T, Eigen::Vector3f>) {
                ctx->emit_vec3(ctx->store, ctx->node_id, nm, val.x(), val.y(), val.z());
            } else if constexpr (std::is_same_v<T, Eigen::Vector4f>) {
                ctx->emit_vec4(ctx->store, ctx->node_id, nm, val.x(), val.y(), val.z(), val.w());
            } else if constexpr (std::is_same_v<T, Eigen::Matrix4f>) {
                ctx->emit_mat4(ctx->store, ctx->node_id, nm, val.data());
            } else if constexpr (std::is_same_v<T, Eigen::Quaternionf>) {
                ctx->emit_quat(ctx->store, ctx->node_id, nm, val.x(), val.y(), val.z(), val.w());
            } else if constexpr (std::is_same_v<T, GpuTexture>) {
                ctx->emit_texture(ctx->store, ctx->node_id, nm, val.id, val.width,
                                  val.height, val.internal_format, val.filter);
            } else if constexpr (std::is_same_v<T, AudioBuffer>) {
                ctx->emit_audio(ctx->store, ctx->node_id, nm, val.data, val.frames,
                                val.channels, val.sample_rate);
            } else if constexpr (std::is_same_v<T, DrawFn>) {
                if (ctx->emit_drawfn)
                    ctx->emit_drawfn(ctx->store, ctx->node_id, nm,
                                     static_cast<const void*>(&val));
            } else if constexpr (std::is_same_v<T, MeshPtr>) {
                if (ctx->emit_mesh)
                    ctx->emit_mesh(ctx->store, ctx->node_id, nm,
                                   static_cast<const void*>(&val));
            }
        }, it->second);
    }
}

void SubgraphNode::push_draw_calls_to(DrawCallCtx* ctx) {
    ctx->calls->insert(ctx->calls->end(),
                       inner_->draw_calls.begin(), inner_->draw_calls.end());
}
