// Copyright 2025 Travis West
#include "signal_graph.hpp"
#include "signal_graph_plan.hpp"
#include "subgraph_node.hpp"
#include "component_registry.hpp"
#include "port_schema_reader.hpp"
#include "port_types.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cctype>

Graph::~Graph() {
    for (auto& n : nodes) {
        if (n.desc && n.desc->destroy) n.desc->destroy(n.data);
    }
}

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
    p += needle.size() - 1;
    int depth = 0;
    bool in_str = false;
    auto start = p;
    for (std::size_t i = p; i < json.size(); ++i) {
        char ch = json[i];
        if (in_str) {
            if (ch == '\\') ++i;
            else if (ch == '"') in_str = false;
        } else if (ch == '"') in_str = true;
        else if (ch == '{') ++depth;
        else if (ch == '}') { if (--depth == 0) return json.substr(start, i - start + 1); }
    }
    return {};
}

static std::vector<std::string> split_array_objects(std::string_view json) {
    std::vector<std::string> result;
    auto p = json.find('[');
    if (p == std::string_view::npos) return result;
    ++p;
    while (p < json.size()) {
        while (p < json.size() && json[p] != '{' && json[p] != ']') ++p;
        if (p >= json.size() || json[p] == ']') break;
        int depth = 0;
        bool in_str = false;
        auto start = p;
        for (std::size_t i = p; i < json.size(); ++i) {
            char ch = json[i];
            if (in_str) {
                if (ch == '\\') ++i;
                else if (ch == '"') in_str = false;
            } else if (ch == '"') in_str = true;
            else if (ch == '{') ++depth;
            else if (ch == '}') {
                if (--depth == 0) {
                    result.push_back(std::string(json.substr(start, i - start + 1)));
                    p = i + 1;
                    break;
                }
            }
        }
    }
    return result;
}

static bool split_dot(std::string_view s, std::string& node, std::string& port) {
    auto dot = s.find('.');
    if (dot == std::string_view::npos) return false;
    node = std::string(s.substr(0, dot));
    port = std::string(s.substr(dot + 1));
    return true;
}

// Strips whitespace outside string literals so standard JSON emitters
// (python json.dumps, pretty-printers) parse identically to compact JSON —
// the needle matchers below assume "key":value with no gaps.
std::string compact_json(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    bool in_str = false, esc = false;
    for (char ch : in) {
        if (in_str) {
            out += ch;
            if (esc) esc = false;
            else if (ch == '\\') esc = true;
            else if (ch == '"') in_str = false;
        } else if (ch == '"') {
            out += ch;
            in_str = true;
        } else if (!std::isspace(static_cast<unsigned char>(ch))) {
            out += ch;
        }
    }
    return out;
}

std::unique_ptr<Graph> parse_graph(const std::string& raw_json, const ComponentRegistry& registry) {
    const std::string json = compact_json(raw_json);
    auto nodes_key = std::string_view(json).find("\"nodes\"");
    if (nodes_key == std::string_view::npos) return nullptr;

    auto arr_start = json.find('[', nodes_key);
    if (arr_start == std::string_view::npos) return nullptr;
    auto node_objs = split_array_objects(std::string_view(json).substr(arr_start));

    auto g = std::make_unique<Graph>();
    for (const auto& obj : node_objs) {
        auto id = find_string(obj, "id");
        if (id.empty()) return nullptr;

        auto inline_graph_json = find_object(obj, "graph");
        if (!inline_graph_json.empty()) {
            auto inner = parse_graph(std::string(inline_graph_json), registry);
            if (!inner) return nullptr;
            std::string inline_type = "__inline_" + std::string(id);
            SubgraphDescriptorPtr subdesc(new SubgraphDescriptor(std::move(inner), inline_type));
            const EyeballsNodeDescriptor* desc = subdesc->descriptor();
            void* data = desc->create();
            if (!data) return nullptr;
            g->nodes.push_back({desc, data, std::string(id)});
            g->owned_descriptors.push_back(std::move(subdesc));
            continue;
        }

        auto type = find_string(obj, "type");
        if (type.empty()) return nullptr;

        const auto* desc = registry.find(std::string(type));
        if (!desc) return nullptr;

        void* data = desc->create();
        if (!data) return nullptr;

        auto params = find_object(obj, "params");
        if (!params.empty() && desc->deserialize)
            desc->deserialize(data, std::string(params).c_str());

        g->nodes.push_back({desc, data, std::string(id)});
    }

    auto kind_of = [&](const std::string& node_id, const std::string& port,
                       bool output) -> std::string {
        for (const auto& n : g->nodes) {
            if (n.id != node_id) continue;
            auto schema = parse_port_schema(n.desc ? n.desc->port_schema : nullptr);
            for (const auto& pi : output ? schema.outputs : schema.inputs)
                if (pi.name == port) return pi.kind;
            return "unknown";  // unlisted port: let the setter decide
        }
        return "unknown";
    };

    auto edges_key = std::string_view(json).find("\"edges\"");
    if (edges_key != std::string_view::npos) {
        auto ea_start = json.find('[', edges_key);
        if (ea_start != std::string_view::npos) {
            for (const auto& eobj : split_array_objects(std::string_view(json).substr(ea_start))) {
                auto from_val = find_string(eobj, "from");
                auto to_val   = find_string(eobj, "to");
                if (from_val.empty() || to_val.empty()) continue;
                Edge e;
                if (!split_dot(from_val, e.from_node, e.from_port)) continue;
                if (!split_dot(to_val,   e.to_node,   e.to_port))   continue;
                // Shared legality (same rules the editor enforces at wire
                // time): a type-illegal edge rejects the whole graph, loudly.
                std::string fk = kind_of(e.from_node, e.from_port, true);
                std::string tk = kind_of(e.to_node,   e.to_port,   false);
                if (!port_types::connection_legal(fk, tk)) {
                    std::fprintf(stderr,
                        "parse_graph: illegal edge %s.%s (%s) -> %s.%s (%s)\n",
                        e.from_node.c_str(), e.from_port.c_str(), fk.c_str(),
                        e.to_node.c_str(), e.to_port.c_str(), tk.c_str());
                    return nullptr;
                }
                g->edges.push_back(std::move(e));
            }
        }
    }

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
        std::fprintf(stderr, "clone_graph: src has owned_descriptors; not cloned\n");
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
    // Auto-inserted boundary mappings, visible but never persisted as
    // topology: derived on every serialize, ignored by parse_graph.
    if (auto z1 = cycle_mappings(g); !z1.empty()) {
        out += ",\"mappings\":[";
        bool first_m = true;
        for (const Edge* e : z1) {
            if (!first_m) out += ',';
            first_m = false;
            out += "{\"kind\":\"z1\",\"from\":\""; out += e->from_node;
            out += '.'; out += e->from_port;
            out += "\",\"to\":\""; out += e->to_node; out += '.'; out += e->to_port;
            out += "\"}";
        }
        out += ']';
    }
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
