// Copyright 2025 Travis West
#pragma once

namespace sky_dome_shaders {

constexpr const char* DOME_VERT = R"(#version 300 es
layout(location=0) in vec3 aPos;
out vec3 vPos;
uniform mat4 uVP;
void main() {
    vPos = aPos;
    vec4 clip = uVP * vec4(aPos, 0.0);
    gl_Position = clip.xyww;
}
)";

constexpr const char* DOME_FRAG = R"(#version 300 es
precision mediump float;
in vec3 vPos;
out vec4 fragColor;
uniform vec4 uHorizon;
uniform vec4 uZenith;
uniform vec3 uBodyDir;
uniform vec4 uBodyColor;
uniform float uBodyCos;
void main() {
    vec3 dir = normalize(vPos);
    float t = clamp(dir.y * 2.0, 0.0, 1.0);
    fragColor = mix(uHorizon, uZenith, t);
    float cosA = dot(dir, normalize(uBodyDir));
    float disc = smoothstep(uBodyCos - 0.001, uBodyCos + 0.001, cosA);
    fragColor = mix(fragColor, uBodyColor, disc * uBodyColor.a);
}
)";

constexpr const char* STAR_VERT = R"(#version 300 es
layout(location=0) in vec3 aPos;
uniform mat4 uVP;
void main() {
    vec4 clip = uVP * vec4(aPos, 0.0);
    gl_Position = clip.xyww;
    gl_PointSize = 2.0;
}
)";

constexpr const char* STAR_FRAG = R"(#version 300 es
precision mediump float;
out vec4 fragColor;
void main() { fragColor = vec4(1.0, 1.0, 1.0, 1.0); }
)";

} // namespace sky_dome_shaders
