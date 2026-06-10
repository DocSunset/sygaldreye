// Copyright 2025 Travis West
#include "signal_graph.hpp"
#include "signal_graph_sort.hpp"
#include <cstring>

void tick_graph(Graph& g, double time_s) {
    g.draw_calls.clear();

    auto order = topo_sort(g.nodes, g.edges);

    for (std::size_t idx : order) {
        auto& n = g.nodes[idx];

        for (const auto& e : g.edges) {
            if (e.to_node != n.id) continue;
            std::string from_key = e.from_node + "." + e.from_port;
            auto it = g.values.find(from_key);
            if (it == g.values.end()) continue;

            std::visit([&](const auto& val) {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, double>) {
                    if (n.desc->set_scalar_in)
                        n.desc->set_scalar_in(n.data, e.to_port.c_str(), val);
                } else if constexpr (std::is_same_v<T, Eigen::Vector2f>) {
                    if (n.desc->set_vec2_in)
                        n.desc->set_vec2_in(n.data, e.to_port.c_str(), val.x(), val.y());
                } else if constexpr (std::is_same_v<T, Eigen::Vector3f>) {
                    if (n.desc->set_vec3_in)
                        n.desc->set_vec3_in(n.data, e.to_port.c_str(),
                                            val.x(), val.y(), val.z());
                } else if constexpr (std::is_same_v<T, Eigen::Vector4f>) {
                    if (n.desc->set_vec4_in)
                        n.desc->set_vec4_in(n.data, e.to_port.c_str(),
                                            val.x(), val.y(), val.z(), val.w());
                } else if constexpr (std::is_same_v<T, Eigen::Matrix4f>) {
                    if (n.desc->set_mat4_in)
                        n.desc->set_mat4_in(n.data, e.to_port.c_str(), val.data());
                } else if constexpr (std::is_same_v<T, Eigen::Quaternionf>) {
                    if (n.desc->set_quat_in)
                        n.desc->set_quat_in(n.data, e.to_port.c_str(),
                                            val.x(), val.y(), val.z(), val.w());
                } else if constexpr (std::is_same_v<T, GpuTexture>) {
                    if (n.desc->set_texture_in)
                        n.desc->set_texture_in(n.data, e.to_port.c_str(),
                                               val.id, val.width, val.height,
                                               val.internal_format, val.filter);
                } else if constexpr (std::is_same_v<T, AudioBuffer>) {
                    if (n.desc->set_audio_in)
                        n.desc->set_audio_in(n.data, e.to_port.c_str(),
                                             val.data, val.frames,
                                             val.channels, val.sample_rate);
                } else if constexpr (std::is_same_v<T, DrawFn>) {
                    if (n.desc->set_drawfn_in)
                        n.desc->set_drawfn_in(n.data, e.to_port.c_str(),
                                              static_cast<const void*>(&val));
                }
            }, it->second);
        }

        if (n.desc->process) n.desc->process(n.data, time_s);

        if (n.desc->push_outputs) {
            EyeballsOutputCtx ctx{};
            ctx.store   = &g.values;
            ctx.node_id = n.id.c_str();
            ctx.emit_scalar = [](void* store, const char* nid, const char* port, double v) {
                auto& m = *static_cast<std::unordered_map<std::string, PortValue>*>(store);
                m[std::string(nid) + "." + port] = v;
            };
            ctx.emit_vec2 = [](void* store, const char* nid, const char* port, float x, float y) {
                auto& m = *static_cast<std::unordered_map<std::string, PortValue>*>(store);
                m[std::string(nid) + "." + port] = Eigen::Vector2f{x, y};
            };
            ctx.emit_vec3 = [](void* store, const char* nid, const char* port,
                               float x, float y, float z) {
                auto& m = *static_cast<std::unordered_map<std::string, PortValue>*>(store);
                m[std::string(nid) + "." + port] = Eigen::Vector3f{x, y, z};
            };
            ctx.emit_vec4 = [](void* store, const char* nid, const char* port,
                               float x, float y, float z, float w) {
                auto& m = *static_cast<std::unordered_map<std::string, PortValue>*>(store);
                m[std::string(nid) + "." + port] = Eigen::Vector4f{x, y, z, w};
            };
            ctx.emit_mat4 = [](void* store, const char* nid, const char* port,
                               const float* c16) {
                auto& m = *static_cast<std::unordered_map<std::string, PortValue>*>(store);
                Eigen::Matrix4f mat;
                std::copy(c16, c16 + 16, mat.data());
                m[std::string(nid) + "." + port] = mat;
            };
            ctx.emit_quat = [](void* store, const char* nid, const char* port,
                               float x, float y, float z, float w) {
                auto& m = *static_cast<std::unordered_map<std::string, PortValue>*>(store);
                m[std::string(nid) + "." + port] = Eigen::Quaternionf{w, x, y, z};
            };
            ctx.emit_texture = [](void* store, const char* nid, const char* port,
                                  unsigned int id, int w, int h,
                                  unsigned int fmt, unsigned int filt) {
                auto& m = *static_cast<std::unordered_map<std::string, PortValue>*>(store);
                m[std::string(nid) + "." + port] = GpuTexture{id, w, h, fmt, filt};
            };
            ctx.emit_drawfn = [](void* store, const char* nid, const char* port,
                                 const void* fn) {
                auto& m = *static_cast<std::unordered_map<std::string, PortValue>*>(store);
                m[std::string(nid) + "." + port] = *static_cast<const DrawFn*>(fn);
            };
            ctx.emit_audio = [](void* store, const char* nid, const char* port,
                                const float* data, int frames, int channels, int rate) {
                auto& m = *static_cast<std::unordered_map<std::string, PortValue>*>(store);
                m[std::string(nid) + "." + port] = AudioBuffer{data, frames, channels, rate};
            };
            n.desc->push_outputs(n.data, &ctx);
        }

        if (n.desc->push_draw_calls) {
            // A node whose draw output feeds an edge is consumed by that
            // consumer (render_target etc.) and skips the global pass.
            bool consumed = false;
            for (const auto& e : g.edges) {
                if (e.from_node != n.id) continue;
                auto it = g.values.find(e.from_node + "." + e.from_port);
                if (it != g.values.end() &&
                    std::holds_alternative<DrawFn>(it->second)) {
                    consumed = true;
                    break;
                }
            }
            if (!consumed) {
                DrawCallCtx ctx{n.id, &g.draw_calls};
                n.desc->push_draw_calls(n.data, &ctx);
            }
        }
    }
}
