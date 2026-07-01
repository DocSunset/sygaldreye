// Copyright 2025 Travis West
#pragma once
#include <string>
#include <vector>

struct PortInfo {
    std::string name;
    std::string kind;  // "scalar", "vec3", "vec4", "mat4", "texture", "audio", etc.
    float min = 0.f;   // slider range; 0/1 when the schema doesn't say
    float max = 1.f;
    // v7 lifting: a port's natural CELL is a pure function of its kind —
    // scalar 1, vec2 2, vec3 3, vec4/quat 4, mat4 16. Whole-payload kinds
    // (audio/span/mesh/surface/drawable/texture/text) consume the payload as a
    // unit and never lift, reported as cell_rank 0 + whole.
    bool whole = false;                       // explicit opt-out (whole<T>) OR a whole-by-kind port
    int cell_rank() const;                    // 0 ⇒ whole-array (no lifting over it)
    bool is_wirable() const { return true; }  // every port wires (DrawFn gone)
};

struct PortSchema {
    std::vector<PortInfo> inputs;
    std::vector<PortInfo> outputs;
};

// Parse the static JSON string from EyeballsNodeDescriptor::port_schema.
// Returns empty PortSchema if port_schema is null or malformed.
PortSchema parse_port_schema(const char* json);
