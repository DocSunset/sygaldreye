// Copyright 2026 Travis West
#pragma once

// Shared GLSL sources for the shader-specific nodes (audit L5: these were
// pasted per-producer). Producers pass them into ShaderData; render_region's
// program cache keys on source content, so every node using a common shader
// shares ONE compiled program.
namespace common_shaders {

// Unlit, per-vertex color. uMVP injected by render_region.
inline constexpr const char* kUnlitVertexColorVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
layout(location=2) in vec4 aColor;
uniform mat4 uMVP;
out vec4 vColor;
void main() {
    vColor = aColor;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";
inline constexpr const char* kUnlitVertexColorFrag = R"(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 fragColor;
void main() { fragColor = vColor; }
)";

// Unlit, single uniform color (uColor).
inline constexpr const char* kUnlitUniformColorVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
void main() { gl_Position = uMVP * vec4(aPos, 1.0); }
)";
inline constexpr const char* kUnlitUniformColorFrag = R"(#version 300 es
precision mediump float;
uniform vec4 uColor;
out vec4 fragColor;
void main() { fragColor = uColor; }
)";

// MSDF text: aPos in world, UV carried in aNormal.xy (TriVertex has no UV).
// Samples uAtlas; uRange is the atlas distance range, uColor tints.
inline constexpr const char* kMsdfTextVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 uMVP;
out vec2 vUV;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vUV = aNormal.xy;
}
)";
inline constexpr const char* kMsdfTextFrag = R"(#version 300 es
precision mediump float;
in vec2 vUV;
uniform sampler2D uAtlas;
uniform float uRange;
uniform vec4 uColor;
out vec4 fragColor;
float median(float r, float g, float b) { return max(min(r, g), min(max(r, g), b)); }
void main() {
    vec3 msd = texture(uAtlas, vUV).rgb;
    float sd = median(msd.r, msd.g, msd.b) - 0.5;
    vec2 unit_range = vec2(uRange) / vec2(textureSize(uAtlas, 0));
    vec2 screen_tex_size = vec2(1.0) / fwidth(vUV);
    float px_range = max(0.5 * dot(unit_range, screen_tex_size), 1.0);
    float alpha = clamp(sd * px_range + 0.5, 0.0, 1.0);
    fragColor = vec4(uColor.rgb, uColor.a * alpha);
}
)";

}  // namespace common_shaders
