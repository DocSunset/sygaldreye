// Copyright 2025 Travis West
#pragma once
#include <string>
#include <vector>

struct PortInfo {
    std::string name;
    std::string kind;  // "scalar", "vec3", "vec4", "mat4", "texture", "draw_call", "audio", etc.
    bool is_drawable() const { return kind == "draw_call"; }
    bool is_wirable()  const { return kind != "draw_call"; }
};

struct PortSchema {
    std::vector<PortInfo> inputs;
    std::vector<PortInfo> outputs;
};

// Parse the static JSON string from EyeballsNodeDescriptor::port_schema.
// Returns empty PortSchema if port_schema is null or malformed.
PortSchema parse_port_schema(const char* json);
