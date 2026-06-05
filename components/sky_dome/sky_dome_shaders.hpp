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

// uSunElevation: -1.0 = midnight, 0.0 = horizon, +1.0 = zenith
// Horizon/zenith colors are computed procedurally from sun elevation.
// uHorizon / uZenith are used as overrides when uUseOverride != 0.
constexpr const char* DOME_FRAG = R"(#version 300 es
precision mediump float;
in vec3 vPos;
out vec4 fragColor;
uniform vec3  uBodyDir;    // sun direction (unit)
uniform vec4  uBodyColor;  // rgb=sun color, a=disc alpha
uniform float uBodyCos;    // cos(angular radius)
uniform float uSunElevation; // -1..+1
uniform int   uUseOverride;
uniform vec4  uHorizon;
uniform vec4  uZenith;

// Procedural sky colors from sun elevation
vec3 sky_horizon(float elev) {
    // Day: blue-white. Dawn/dusk: orange-red. Night: near black.
    float day   = clamp(elev * 2.0, 0.0, 1.0);
    float dusk  = 1.0 - abs(elev) * 3.5;
    dusk = clamp(dusk, 0.0, 1.0);
    dusk = dusk * dusk;
    vec3 dayH   = vec3(0.62, 0.78, 0.96);
    vec3 duskH  = vec3(1.00, 0.38, 0.08);
    vec3 nightH = vec3(0.02, 0.03, 0.07);
    vec3 h = mix(nightH, dayH, day);
    h = mix(h, duskH, dusk);
    return h;
}

vec3 sky_zenith(float elev) {
    float day   = clamp(elev * 2.0, 0.0, 1.0);
    float dusk  = 1.0 - abs(elev) * 4.0;
    dusk = clamp(dusk, 0.0, 1.0);
    dusk = dusk * dusk;
    vec3 dayZ   = vec3(0.10, 0.22, 0.65);
    vec3 duskZ  = vec3(0.20, 0.10, 0.28);
    vec3 nightZ = vec3(0.01, 0.01, 0.04);
    vec3 z = mix(nightZ, dayZ, day);
    z = mix(z, duskZ, dusk * 0.7);
    return z;
}

void main() {
    vec3 dir = normalize(vPos);
    float t = clamp(dir.y * 2.0, 0.0, 1.0);

    vec4 horizon, zenith;
    if (uUseOverride != 0) {
        horizon = uHorizon;
        zenith  = uZenith;
    } else {
        horizon = vec4(sky_horizon(uSunElevation), 1.0);
        zenith  = vec4(sky_zenith(uSunElevation),  1.0);
    }
    fragColor = mix(horizon, zenith, t * t);

    // Sun disc with soft edge + glow halo
    vec3  sunDir = normalize(uBodyDir);
    float cosA   = dot(dir, sunDir);
    float disc   = smoothstep(uBodyCos - 0.002, uBodyCos + 0.002, cosA);
    // Glow: falloff beyond disc edge
    float glow   = clamp((cosA - (uBodyCos - 0.08)) / 0.08, 0.0, 1.0);
    glow = glow * glow * (1.0 - disc) * 0.35;
    float dayFactor = clamp(uSunElevation * 3.0 + 0.5, 0.0, 1.0);
    fragColor.rgb = mix(fragColor.rgb, uBodyColor.rgb, (disc + glow) * uBodyColor.a * dayFactor);
}
)";

constexpr const char* STAR_VERT = R"(#version 300 es
precision mediump float;
layout(location=0) in vec3 aPos;
uniform mat4  uVP;
uniform float uStarAlpha;
void main() {
    vec4 clip = uVP * vec4(aPos, 0.0);
    gl_Position = clip.xyww;
    gl_PointSize = 2.0;
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

} // namespace sky_dome_shaders
