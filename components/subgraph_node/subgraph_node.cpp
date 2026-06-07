// Copyright 2025 Travis West
#include "subgraph_node.hpp"
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
        const auto* d = it->desc;
        void* data    = it->data;
        const char* p = decl.port.c_str();
        std::visit([&](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, double>) {
                if (d->set_scalar_in) d->set_scalar_in(data, p, val);
            } else if constexpr (std::is_same_v<T, Eigen::Vector2f>) {
                if (d->set_vec2_in) d->set_vec2_in(data, p, val.x(), val.y());
            } else if constexpr (std::is_same_v<T, Eigen::Vector3f>) {
                if (d->set_vec3_in) d->set_vec3_in(data, p, val.x(), val.y(), val.z());
            } else if constexpr (std::is_same_v<T, Eigen::Vector4f>) {
                if (d->set_vec4_in) d->set_vec4_in(data, p, val.x(), val.y(), val.z(), val.w());
            } else if constexpr (std::is_same_v<T, Eigen::Matrix4f>) {
                if (d->set_mat4_in) d->set_mat4_in(data, p, val.data());
            } else if constexpr (std::is_same_v<T, Eigen::Quaternionf>) {
                if (d->set_quat_in) d->set_quat_in(data, p, val.x(), val.y(), val.z(), val.w());
            } else if constexpr (std::is_same_v<T, GpuTexture>) {
                if (d->set_texture_in)
                    d->set_texture_in(data, p, val.id, val.width, val.height,
                                      val.internal_format, val.filter);
            } else if constexpr (std::is_same_v<T, AudioBuffer>) {
                if (d->set_audio_in)
                    d->set_audio_in(data, p, val.data, val.frames, val.channels, val.sample_rate);
            }
        }, cv->second);
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
            }
        }, it->second);
    }
}

void SubgraphNode::push_draw_calls_to(DrawCallCtx* ctx) {
    ctx->calls->insert(ctx->calls->end(),
                       inner_->draw_calls.begin(), inner_->draw_calls.end());
}
