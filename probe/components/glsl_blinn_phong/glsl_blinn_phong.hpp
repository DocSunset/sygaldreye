// Copyright 2025 Travis West
#pragma once

// GLSL helper function: Blinn-Phong diffuse + specular factors.
// Insert into fragment shader source before void main().
// Returns vec2(diffuse_factor, specular_factor).
constexpr const char* BLINN_PHONG_GLSL =
    "vec2 blinn_phong(vec3 N, vec3 L, vec3 V, float shininess) {\n"
    "    vec3 H = normalize(L + V);\n"
    "    return vec2(max(dot(N, L), 0.0), pow(max(dot(N, H), 0.0), shininess));\n"
    "}\n";
