// Copyright 2025 Travis West
#include "subgraph_node.hpp"
#include "signal_graph_plan.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string_view>

// ── SubgraphNode ─────────────────────────────────────────────────────────────

SubgraphNode::SubgraphNode(std::unique_ptr<Graph> inner)
    : inner_(std::move(inner)) {
    // Plan + wire the inner graph NOW: its wire_plan resets every v6 input,
    // so it must run before the outer graph forwards wires into us (the
    // lazy build at first tick would wipe outer connections).
    if (!inner_->plan) {
        inner_->plan = build_plan(*inner_);
        wire_plan(*inner_);
    }
}

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

int SubgraphNode::connect_inlet(const char* name, const void* src) {
    int hit = 0;
    for (const auto& decl : inner_->inlets) {
        if (decl.name != name) continue;
        auto it = std::find_if(inner_->nodes.begin(), inner_->nodes.end(),
                               [&](const NodeInstance& n) { return n.id == decl.node; });
        if (it == inner_->nodes.end() || !it->desc->connect) continue;
        hit |= it->desc->connect(it->data, decl.port.c_str(), src);
    }
    return hit;
}

const void* SubgraphNode::outlet_ptr(const char* name) const {
    for (const auto& decl : inner_->outlets) {
        if (decl.name != name) continue;
        auto it = std::find_if(inner_->nodes.begin(), inner_->nodes.end(),
                               [&](const NodeInstance& n) { return n.id == decl.node; });
        if (it == inner_->nodes.end() || !it->desc->output_ptr) continue;
        return it->desc->output_ptr(it->data, decl.port.c_str());
    }
    return nullptr;
}

void SubgraphNode::push_draw_calls_to(DrawCallCtx* ctx) {
    ctx->calls->insert(ctx->calls->end(),
                       inner_->draw_calls.begin(), inner_->draw_calls.end());
}

// ── inlet-params (rung 1) ────────────────────────────────────────────────────

void SubgraphNode::deserialize_params(const char* json) {
    std::string_view s{json ? json : ""};
    auto pos = s.find('{');
    if (pos == std::string_view::npos) return;
    ++pos;
    while (pos < s.size()) {
        while (pos < s.size() && s[pos] != '"' && s[pos] != '}') ++pos;
        if (pos >= s.size() || s[pos] == '}') break;
        auto key_end = s.find('"', ++pos);
        if (key_end == std::string_view::npos) break;
        std::string key{s.substr(pos, key_end - pos)};
        pos = s.find(':', key_end);
        if (pos == std::string_view::npos) break;
        ++pos;
        while (pos < s.size() && s[pos] == ' ') ++pos;
        PortValue v;
        if (pos < s.size() && s[pos] == '"') {
            auto q = pos + 1;
            while (q < s.size() && !(s[q] == '"' && s[q - 1] != '\\')) ++q;
            std::string txt;
            for (auto i = pos + 1; i < q; ++i) {
                if (s[i] == '\\' && i + 1 < q) {
                    char c = s[++i];
                    txt += (c == 'n') ? '\n' : (c == 't') ? '\t' : c;
                } else txt += s[i];
            }
            v   = std::move(txt);
            pos = (q < s.size()) ? q + 1 : q;
        } else {
            auto end = s.find_first_of(",}", pos);
            if (end == std::string_view::npos) end = s.size();
            v   = strtod(s.data() + pos, nullptr);
            pos = end;
        }
        param_defaults_[key] = v;
        cache_inlet(key, std::move(v));
    }
}

std::string SubgraphNode::serialize_params() const {
    std::string out = "{";
    bool first = true;
    for (const auto& [k, v] : param_defaults_) {
        if (!first) out += ',';
        first = false;
        out += '"' + k + "\":";
        if (const double* d = std::get_if<double>(&v)) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%g", *d);
            out += buf;
        } else if (const std::string* t = std::get_if<std::string>(&v)) {
            out += '"';
            for (char c : *t) {
                if (c == '\n')      out += "\\n";
                else if (c == '\t') out += "\\t";
                else {
                    if (c == '"' || c == '\\') out += '\\';
                    out += c;
                }
            }
            out += '"';
        } else out += "0";
    }
    return out + "}";
}
