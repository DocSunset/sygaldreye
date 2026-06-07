// Copyright 2025 Travis West
#include "signal_graph.hpp"
#include "subgraph_node.hpp"  // complete SubgraphDescriptor for owned_descriptors dtor
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <queue>
#include <unordered_map>

Graph::~Graph() {
    for (auto& n : nodes) {
        if (n.desc && n.desc->destroy) n.desc->destroy(n.data);
    }
}

// Minimal JSON helpers — find next occurrence of key in object JSON
static std::string_view find_string(std::string_view json, std::string_view key) {
    std::string needle = "\"";
    needle += key; needle += "\":\"";
    auto p = json.find(needle);
    if (p == std::string_view::npos) return {};
    p += needle.size();
    auto end = json.find('"', p);
    if (end == std::string_view::npos) return {};
    return json.substr(p, end - p);
}

static std::string_view find_object(std::string_view json, std::string_view key) {
    std::string needle = "\"";
    needle += key; needle += "\":{";
    auto p = json.find(needle);
    if (p == std::string_view::npos) return {};
    p += needle.size() - 1; // points at '{'
    int depth = 0;
    auto start = p;
    for (std::size_t i = p; i < json.size(); ++i) {
        if (json[i] == '{') ++depth;
        else if (json[i] == '}') { if (--depth == 0) return json.substr(start, i - start + 1); }
    }
    return {};
}

// Extract array of objects from json["nodes"]
static std::vector<std::string> split_array_objects(std::string_view json) {
    std::vector<std::string> result;
    auto p = json.find('[');
    if (p == std::string_view::npos) return result;
    ++p;
    while (p < json.size()) {
        while (p < json.size() && json[p] != '{' && json[p] != ']') ++p;
        if (p >= json.size() || json[p] == ']') break;
        int depth = 0;
        auto start = p;
        for (std::size_t i = p; i < json.size(); ++i) {
            if (json[i] == '{') ++depth;
            else if (json[i] == '}') { if (--depth == 0) { result.push_back(std::string(json.substr(start, i - start + 1))); p = i + 1; break; } }
        }
    }
    return result;
}

// Split "node.port" on first '.' into (node, port). Returns false if no dot found.
static bool split_dot(std::string_view s, std::string& node, std::string& port) {
    auto dot = s.find('.');
    if (dot == std::string_view::npos) return false;
    node = std::string(s.substr(0, dot));
    port = std::string(s.substr(dot + 1));
    return true;
}

std::unique_ptr<Graph> parse_graph(const std::string& json, const ComponentRegistry& registry) {
    auto nodes_key = std::string_view(json).find("\"nodes\"");
    if (nodes_key == std::string_view::npos) return nullptr;

    // Find nodes array
    auto arr_start = json.find('[', nodes_key);
    if (arr_start == std::string_view::npos) return nullptr;
    auto node_objs = split_array_objects(std::string_view(json).substr(arr_start));

    auto g = std::make_unique<Graph>();
    for (const auto& obj : node_objs) {
        auto id   = find_string(obj, "id");
        auto type = find_string(obj, "type");
        if (id.empty() || type.empty()) return nullptr;

        const auto* desc = registry.find(std::string(type));
        if (!desc) return nullptr;

        void* data = desc->create();
        if (!data) return nullptr;

        auto params = find_object(obj, "params");
        if (!params.empty() && desc->deserialize)
            desc->deserialize(data, std::string(params).c_str());

        g->nodes.push_back({desc, data, std::string(id)});
    }

    // Parse edges array
    auto edges_key = std::string_view(json).find("\"edges\"");
    if (edges_key != std::string_view::npos) {
        auto ea_start = json.find('[', edges_key);
        if (ea_start != std::string_view::npos) {
            auto edge_objs = split_array_objects(std::string_view(json).substr(ea_start));
            for (const auto& eobj : edge_objs) {
                auto from_val = find_string(eobj, "from");
                auto to_val   = find_string(eobj, "to");
                if (from_val.empty() || to_val.empty()) continue;
                Edge e;
                if (!split_dot(from_val, e.from_node, e.from_port)) continue;
                if (!split_dot(to_val,   e.to_node,   e.to_port))   continue;
                g->edges.push_back(std::move(e));
            }
        }
    }

    // Parse inlets array
    auto inlets_key = std::string_view(json).find("\"inlets\"");
    if (inlets_key != std::string_view::npos) {
        auto ia_start = json.find('[', inlets_key);
        if (ia_start != std::string_view::npos) {
            for (const auto& obj : split_array_objects(std::string_view(json).substr(ia_start))) {
                InletDecl d;
                d.name = find_string(obj, "name");
                d.node = find_string(obj, "node");
                d.port = find_string(obj, "port");
                g->inlets.push_back(std::move(d));
            }
        }
    }

    // Parse outlets array
    auto outlets_key = std::string_view(json).find("\"outlets\"");
    if (outlets_key != std::string_view::npos) {
        auto oa_start = json.find('[', outlets_key);
        if (oa_start != std::string_view::npos) {
            for (const auto& obj : split_array_objects(std::string_view(json).substr(oa_start))) {
                OutletDecl d;
                d.name = find_string(obj, "name");
                d.node = find_string(obj, "node");
                d.port = find_string(obj, "port");
                g->outlets.push_back(std::move(d));
            }
        }
    }

    return g;
}

std::unique_ptr<Graph> clone_graph(const Graph& src) {
    auto g = std::make_unique<Graph>();
    for (const auto& n : src.nodes) {
        void* data = n.desc->create();
        if (n.desc->serialize && n.desc->deserialize) {
            const char* s = n.desc->serialize(n.data);
            n.desc->deserialize(data, s);
            if (n.desc->free_str) n.desc->free_str(s);
        }
        g->nodes.push_back({n.desc, data, n.id});
    }
    g->edges   = src.edges;
    g->inlets  = src.inlets;
    g->outlets = src.outlets;
    if (!src.owned_descriptors.empty())
        std::fprintf(stderr,
            "clone_graph: src has owned_descriptors; not cloned\n");
    return g;
}

std::string serialize_graph(const Graph& g) {
    std::string out = "{\"nodes\":[";
    bool first = true;
    for (const auto& n : g.nodes) {
        if (!first) out += ',';
        first = false;
        out += "{\"id\":\""; out += n.id; out += "\",\"type\":\"";
        out += n.desc->type_name; out += "\"";
        if (n.desc->serialize) {
            const char* s = n.desc->serialize(n.data);
            out += ",\"params\":"; out += s;
            if (n.desc->free_str) n.desc->free_str(s);
        }
        if (n.desc->source_header || n.desc->source_cpp) {
            out += ",\"provenance\":{\"kind\":\"local\"";
            if (n.desc->source_header) {
                out += ",\"header\":\""; out += n.desc->source_header; out += '"';
            }
            if (n.desc->source_cpp) {
                out += ",\"source\":\""; out += n.desc->source_cpp; out += '"';
            }
            out += '}';
        }
        out += '}';
    }
    out += "],\"edges\":[";
    bool first_edge = true;
    for (const auto& e : g.edges) {
        if (!first_edge) out += ',';
        first_edge = false;
        out += "{\"from\":\""; out += e.from_node; out += '.'; out += e.from_port;
        out += "\",\"to\":\""; out += e.to_node;   out += '.'; out += e.to_port;
        out += "\"}";
    }
    out += ']';
    if (!g.inlets.empty()) {
        out += ",\"inlets\":[";
        bool first_in = true;
        for (const auto& d : g.inlets) {
            if (!first_in) out += ',';
            first_in = false;
            out += "{\"name\":\""; out += d.name;
            out += "\",\"node\":\""; out += d.node;
            out += "\",\"port\":\""; out += d.port; out += "\"}";
        }
        out += ']';
    }
    if (!g.outlets.empty()) {
        out += ",\"outlets\":[";
        bool first_out = true;
        for (const auto& d : g.outlets) {
            if (!first_out) out += ',';
            first_out = false;
            out += "{\"name\":\""; out += d.name;
            out += "\",\"node\":\""; out += d.node;
            out += "\",\"port\":\""; out += d.port; out += "\"}";
        }
        out += ']';
    }
    out += '}';
    return out;
}

// Kahn's algorithm topological sort. Returns indices into nodes in dependency order.
// Appends remaining cycle nodes in insertion order if a cycle is found.
static std::vector<std::size_t> topo_sort(
    const std::vector<NodeInstance>& nodes,
    const std::vector<Edge>& edges)
{
    std::unordered_map<std::string, std::size_t> idx_map;
    idx_map.reserve(nodes.size());
    for (std::size_t i = 0; i < nodes.size(); ++i)
        idx_map[nodes[i].id] = i;

    std::vector<int> in_deg(nodes.size(), 0);
    for (const auto& e : edges) {
        auto it_to = idx_map.find(e.to_node);
        if (it_to == idx_map.end()) continue;
        auto it_from = idx_map.find(e.from_node);
        if (it_from == idx_map.end()) continue;
        ++in_deg[it_to->second];
    }

    std::queue<std::size_t> q;
    for (std::size_t i = 0; i < nodes.size(); ++i)
        if (in_deg[i] == 0) q.push(i);

    std::vector<std::size_t> order;
    order.reserve(nodes.size());
    while (!q.empty()) {
        auto i = q.front(); q.pop();
        order.push_back(i);
        for (const auto& e : edges) {
            if (e.from_node != nodes[i].id) continue;
            auto it = idx_map.find(e.to_node);
            if (it == idx_map.end()) continue;
            if (--in_deg[it->second] == 0) q.push(it->second);
        }
    }
    // Cycle fallback: append nodes not yet ordered
    if (order.size() < nodes.size()) {
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (std::find(order.begin(), order.end(), i) == order.end())
                order.push_back(i);
        }
    }
    return order;
}

void tick_graph(Graph& g, double time_s) {
    g.draw_calls.clear();

    auto order = topo_sort(g.nodes, g.edges);

    for (std::size_t idx : order) {
        auto& n = g.nodes[idx];

        // Apply incoming edge values from value store
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
                }
            }, it->second);
        }

        // Process
        if (n.desc->process) n.desc->process(n.data, time_s);

        // Collect typed outputs into value store
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
            ctx.emit_audio = [](void* store, const char* nid, const char* port,
                                const float* data, int frames, int channels, int rate) {
                auto& m = *static_cast<std::unordered_map<std::string, PortValue>*>(store);
                m[std::string(nid) + "." + port] = AudioBuffer{data, frames, channels, rate};
            };
            n.desc->push_outputs(n.data, &ctx);
        }

        // Collect draw calls
        if (n.desc->push_draw_calls) {
            DrawCallCtx ctx{n.id, &g.draw_calls};
            n.desc->push_draw_calls(n.data, &ctx);
        }
    }
}
