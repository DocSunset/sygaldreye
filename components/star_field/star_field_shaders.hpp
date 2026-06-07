// Copyright 2025 Travis West
#pragma once

namespace star_field_shaders {

// Stars are generated procedurally from gl_VertexID; no VBO needed.
constexpr const char* STAR_VERT = R"(#version 300 es
precision mediump float;
uniform mat4  uVP;
uniform float uStarAlpha;
uniform float uRadius;

uint uhash(uint x) {
    x = ((x >> 16u) ^ x) * 0x45d9f3bu;
    x = ((x >> 16u) ^ x) * 0x45d9f3bu;
    return (x >> 16u) ^ x;
}
float fhash(uint x) { return float(uhash(x)) * (1.0 / 4294967296.0); }

void main() {
    uint  id    = uint(gl_VertexID);
    float theta = 6.28318530 * fhash(id);
    float u     = fhash(id + 2000u) * 2.0 - 1.0;
    float r     = sqrt(max(0.0, 1.0 - u * u));
    vec3  pos   = uRadius * vec3(cos(theta) * r, abs(u), sin(theta) * r);
    vec4  clip  = uVP * vec4(pos, 0.0);
    gl_Position  = clip.xyww;
    gl_PointSize = 1.0 + fhash(id + 4000u) * 2.0;
}
)";

constexpr const char* STAR_FRAG = R"(#version 300 es
precision mediump float;
uniform float uStarAlpha;
out vec4 fragColor;
void main() {
    // Soft disc point sprite
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float d = dot(uv, uv);
    float a = smoothstep(1.0, 0.3, d) * uStarAlpha;
    fragColor = vec4(1.0, 1.0, 1.0, a);
}
)";

} // namespace star_field_shaders
