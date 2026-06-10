// Copyright 2025 Travis West
#include "port_schema_reader.hpp"
#include <cstdlib>
#include <cstring>
#include <string_view>

// Find value of a string key in a JSON object fragment.
static std::string_view find_str(std::string_view json, std::string_view key) {
    std::string needle = "\"";
    needle += key; needle += "\":\"";
    auto p = json.find(needle);
    if (p == std::string_view::npos) return {};
    p += needle.size();
    auto end = json.find('"', p);
    if (end == std::string_view::npos) return {};
    return json.substr(p, end - p);
}

// Split a JSON array of objects into individual object strings.
static std::vector<std::string> split_objects(std::string_view json) {
    std::vector<std::string> result;
    auto p = json.find('[');
    if (p == std::string_view::npos) return result;
    ++p;
    while (p < json.size()) {
        while (p < json.size() && json[p] != '{' && json[p] != ']') ++p;
        if (p >= json.size() || json[p] == ']') break;
        int depth = 0;
        auto start = p;
        for (auto i = p; i < json.size(); ++i) {
            if (json[i] == '{') ++depth;
            else if (json[i] == '}') {
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

static std::vector<PortInfo> parse_ports(std::string_view json, std::string_view key) {
    std::string needle = "\"";
    needle += key; needle += "\":[";
    auto p = json.find(needle);
    if (p == std::string_view::npos) return {};
    p += needle.size() - 1; // points at '['
    auto objs = split_objects(json.substr(p));
    std::vector<PortInfo> ports;
    ports.reserve(objs.size());
    for (const auto& obj : objs) {
        auto name = find_str(obj, "name");
        auto kind = find_str(obj, "kind");
        if (name.empty()) continue;
        PortInfo info{std::string(name), std::string(kind), 0.f, 1.f};
        auto num = [&](std::string_view key, float fallback) -> float {
            std::string needle = "\"" + std::string(key) + "\":";
            auto q = obj.find(needle);
            if (q == std::string::npos) return fallback;
            return std::strtof(obj.c_str() + q + needle.size(), nullptr);
        };
        info.min = num("min", 0.f);
        info.max = num("max", 1.f);
        ports.push_back(std::move(info));
    }
    return ports;
}

PortSchema parse_port_schema(const char* json) {
    if (!json) return {};
    std::string_view sv(json);
    PortSchema s;
    s.inputs  = parse_ports(sv, "inputs");
    s.outputs = parse_ports(sv, "outputs");
    return s;
}
