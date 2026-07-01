// Copyright 2025 Travis West
#include "sky_dome.hpp"

#include <cmath>

#include "sphere_geometry.hpp"
#include "tri_mesh.hpp"

namespace {

// Dome at a large finite radius (kDomeRadius, inside the 1000 far plane) with
// ordinary depth. This is order-independent: the sky depth-sorts behind closer
// geometry whether it draws before or after. vPos is the (unscaled) direction
// for the gradient; normalize() makes the radius cancel.
constexpr const char* kVert = R"(#version 300 es
layout(location=0) in vec3 aPos;
out vec3 vPos;
uniform mat4 uMVP;
void main() {
    vPos = aPos;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

// Procedural sky gradient (horizon→zenith from sun elevation) + sun disc/glow.
// Override path dropped — always procedural (single user, no callers used it).
constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
in vec3 vPos;
out vec4 fragColor;
uniform vec3  uBodyDir;       // sun direction (unit)
uniform vec4  uBodyColor;     // rgb=sun color, a=disc alpha
uniform float uBodyCos;       // cos(angular radius)
uniform float uSunElevation;  // -1..+1

vec3 sky_horizon(float elev) {
    float day  = clamp(elev * 2.0, 0.0, 1.0);
    float dusk = clamp(1.0 - abs(elev) * 3.5, 0.0, 1.0);
    dusk = dusk * dusk;
    vec3 h = mix(vec3(0.02, 0.03, 0.07), vec3(0.62, 0.78, 0.96), day);
    return mix(h, vec3(1.00, 0.38, 0.08), dusk);
}
vec3 sky_zenith(float elev) {
    float day  = clamp(elev * 2.0, 0.0, 1.0);
    float dusk = clamp(1.0 - abs(elev) * 4.0, 0.0, 1.0);
    dusk = dusk * dusk;
    vec3 z = mix(vec3(0.01, 0.01, 0.04), vec3(0.10, 0.22, 0.65), day);
    return mix(z, vec3(0.20, 0.10, 0.28), dusk * 0.7);
}
void main() {
    vec3  dir = normalize(vPos);
    float t   = clamp(dir.y * 2.0, 0.0, 1.0);
    vec4 horizon = vec4(sky_horizon(uSunElevation), 1.0);
    vec4 zenith  = vec4(sky_zenith(uSunElevation),  1.0);
    fragColor = mix(horizon, zenith, t * t);

    vec3  sunDir = normalize(uBodyDir);
    float cosA   = dot(dir, sunDir);
    float disc   = smoothstep(uBodyCos - 0.002, uBodyCos + 0.002, cosA);
    float glow   = clamp((cosA - (uBodyCos - 0.08)) / 0.08, 0.0, 1.0);
    glow = glow * glow * (1.0 - disc) * 0.35;
    float dayFactor = clamp(uSunElevation * 3.0 + 0.5, 0.0, 1.0);
    fragColor.rgb = mix(fragColor.rgb, uBodyColor.rgb, (disc + glow) * uBodyColor.a * dayFactor);
}
)";

constexpr float kDomeRadius = 900.0f;  // inside the 1000 far plane

MeshPtr make_dome() {
    SphereGeometry g = make_sphere(32, 16);
    auto m = std::make_shared<TriMeshData>();
    m->vertices.reserve(g.vertices.size());
    for (auto const& v : g.vertices) {
        TriVertex t;
        t.position = v.position * kDomeRadius;
        t.normal = v.normal;
        t.color = Eigen::Vector4f(1.f, 1.f, 1.f, 1.f);
        m->vertices.push_back(t);
    }
    m->indices.reserve(g.indices.size());
    for (uint16_t i : g.indices) m->indices.push_back(static_cast<uint32_t>(i));
    return m;
}

}  // namespace

void SkyDome::operator()(double /*time_s*/) {
    if (!dome_) dome_ = make_dome();
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});

    const float el = endpoints.sun_elevation.get();
    constexpr float az = 0.0f;  // no azimuth input yet
    // to_sun: direction from the ground toward the sun (used for the disc).
    const Eigen::Vector3f to_sun{
        std::cos(el) * std::sin(az), std::sin(el), std::cos(el) * std::cos(az)};

    Mesh m;
    m.geometry = dome_;
    m.mode = Primitive::Triangles;
    endpoints.mesh.value = std::move(m);

    Surface s;
    s.shader = shader_;
    s.depth_test = true;  // far geometry — depth-sorts behind everything closer
    s.depth_write = true;
    s.cull_back = false;  // viewed from inside the dome; render the inner wall
    s.uniforms.push_back({"uBodyDir", to_sun.normalized().eval()});  // disc at the sun
    s.uniforms.push_back({"uBodyColor", Eigen::Vector4f(1.f, 0.98f, 0.85f, 1.f)});
    s.uniforms.push_back({"uBodyCos", std::cos(0.012f)});
    s.uniforms.push_back({"uSunElevation", el});
    endpoints.surface.value = std::move(s);

    endpoints.sun_elevation_out.value = el;
    endpoints.sun_azimuth_out.value = az;
    // sun_dir output is the light-travel direction (downward) — what lighting
    // shaders consume as uSunDir (lightDir = -uSunDir). Matches the lit nodes'
    // downward defaults.
    endpoints.sun_dir.value = -to_sun;
    endpoints.sun_color.value = Eigen::Vector4f(1.f, 0.95f, 0.8f, 1.f);
}
