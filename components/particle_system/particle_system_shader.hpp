#pragma once

namespace particle_system_shader {

constexpr const char* const VERT = R"(#version 300 es
layout(location=0) in vec2 aQuad;
layout(location=1) in vec3 aPos;
layout(location=2) in float aSize;
layout(location=3) in vec4 aColor;
uniform mat4 uVP;
uniform vec3 uRight;
uniform vec3 uUp;
out vec4 vColor;
void main() {
    vec3 worldPos = aPos + uRight * aQuad.x * aSize + uUp * aQuad.y * aSize;
    gl_Position = uVP * vec4(worldPos, 1.0);
    vColor = aColor;
}
)";

constexpr const char* const FRAG = R"(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 fragColor;
void main() { fragColor = vColor; }
)";

} // namespace particle_system_shader
