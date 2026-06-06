// Copyright 2025 Travis West
#include "signal_graph.hpp"
#include <cstring>

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
        out += '}';
    }
    out += "],\"edges\":[]}";
    return out;
}

void tick_graph(Graph& g, double time_s) {
    for (auto& n : g.nodes) {
        if (n.desc->process) n.desc->process(n.data, time_s);
    }
}
