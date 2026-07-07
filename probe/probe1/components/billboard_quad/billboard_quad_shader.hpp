#pragma once

namespace billboard_quad_shader {

constexpr const char* const VERT = R"(#version 300 es
layout(location=0) in vec2 aQuad;
layout(location=1) in vec3 aPos;
layout(location=2) in vec2 aSize;
layout(location=3) in vec4 aColor;
uniform mat4 uVP;
uniform vec3 uRight;
uniform vec3 uUp;
out vec4 vColor;
void main() {
    vec3 wp = aPos + uRight * aQuad.x * aSize.x + uUp * aQuad.y * aSize.y;
    gl_Position = uVP * vec4(wp, 1.0);
    vColor = aColor;
}
)";

constexpr const char* const FRAG = R"(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 fragColor;
void main() { fragColor = vColor; }
)";

} // namespace billboard_quad_shader
