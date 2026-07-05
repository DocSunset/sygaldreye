// Copyright 2025 Travis West
#include "rubber_band_controller.hpp"

#include <Eigen/Geometry>
#include <memory>
#include <span>

#include "cylinder_mesh.hpp"
#include "grab_detector.hpp"
#include "sphere_geometry.hpp"
#include "tri_mesh.hpp"

namespace {

constexpr float kSphereRadius = 0.025F;
constexpr float kBandRadius = 0.005F;
constexpr float kDefaultOffset = 0.1F;
constexpr int kSphereLon = 16;
constexpr int kSphereLat = 8;
constexpr int kCylSlices = 12;

// Lit, per-vertex-color program — spheres read as 3D. render_region injects
// uMVP + uViewPos. Geometry is world-space (no model matrix in the shader).
constexpr const char* kVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec4 aColor;
uniform mat4 uMVP;
out vec3 vNormal;
out vec4 vColor;
void main() { vNormal = aNormal; vColor = aColor; gl_Position = uMVP * vec4(aPos, 1.0); }
)";
constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
in vec3 vNormal;
in vec4 vColor;
out vec4 fragColor;
void main() {
    float d = max(dot(normalize(vNormal), normalize(vec3(0.3, 0.8, 0.5))), 0.0);
    fragColor = vec4(vColor.rgb * (0.3 + 0.7 * d), vColor.a);
}
)";

void append(
    TriMeshData& m,
    const SphereGeometry& geo,
    const Eigen::Matrix4f& model,
    const Eigen::Vector4f& color) {
    Eigen::Matrix3f nrm = model.topLeftCorner<3, 3>().inverse().transpose();
    uint32_t base = uint32_t(m.vertices.size());
    for (const auto& v : geo.vertices)
        m.vertices.push_back(
            {(model * v.position.homogeneous()).head<3>(), (nrm * v.normal).normalized(), color});
    for (uint16_t i : geo.indices) m.indices.push_back(base + i);
}

void append(
    TriMeshData& m,
    const TriMeshData& geo,
    const Eigen::Matrix4f& model,
    const Eigen::Vector4f& color) {
    Eigen::Matrix3f nrm = model.topLeftCorner<3, 3>().inverse().transpose();
    uint32_t base = uint32_t(m.vertices.size());
    for (const auto& v : geo.vertices)
        m.vertices.push_back(
            {(model * v.position.homogeneous()).head<3>(), (nrm * v.normal).normalized(), color});
    for (uint32_t i : geo.indices) m.indices.push_back(base + i);
}

Eigen::Matrix4f sphere_model(Eigen::Vector3f pos) {
    return (Eigen::Translation3f(pos) * Eigen::Scaling(kSphereRadius)).matrix();
}

}  // namespace

void RubberBandController::operator()(double) {
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});

    const Eigen::Vector3f anchor{
        endpoints.anchor_x.get(), endpoints.anchor_y.get(), endpoints.anchor_z.get()};
    if (!seeded_) {
        targets_[0].position = anchor;
        targets_[0].radius = kSphereRadius;
        targets_[1].position = anchor + Eigen::Vector3f{0.f, kDefaultOffset, 0.f};
        targets_[1].radius = kSphereRadius;
        seeded_ = true;
    }

    std::array<HandState, 2> hands{};
    hands[0].position = endpoints.left_pos.get();
    hands[0].valid = endpoints.left_pos.src != nullptr;
    hands[0].grip_pressed = endpoints.left_grip.get() > 0.5f;
    hands[1].position = endpoints.right_pos.get();
    hands[1].valid = endpoints.right_pos.src != nullptr;
    hands[1].grip_pressed = endpoints.right_grip.get() > 0.5f;

    const Eigen::Vector3f prev_anchor = targets_[0].position;
    update_grabs(std::span<const HandState>(hands), std::span<GrabTarget>(targets_));
    auto follow = [&](int t) {
        if (targets_[t].grabbed && targets_[t].grabbing_hand >= 0)
            targets_[t].position =
                hands[size_t(targets_[t].grabbing_hand)].position + targets_[t].grab_offset;
    };
    follow(0);
    if (targets_[0].grabbed && !targets_[1].grabbed)
        targets_[1].position += targets_[0].position - prev_anchor;  // anchor drags both
    follow(1);

    endpoints.offset.value = targets_[1].position - targets_[0].position;

    static const SphereGeometry sphere = make_sphere(kSphereLon, kSphereLat);
    static const MeshPtr cyl = make_cylinder(kCylSlices);
    auto out = std::make_shared<TriMeshData>();
    append(*out, sphere, sphere_model(targets_[0].position), {1.f, 0.8f, 0.f, 1.f});
    append(*out, sphere, sphere_model(targets_[1].position), {0.2f, 0.6f, 1.f, 1.f});
    append(
        *out,
        *cyl,
        cylinder_transform(targets_[0].position, targets_[1].position, kBandRadius),
        {0.9f, 0.2f, 0.2f, 1.f});

    Mesh m;
    m.geometry = out;
    m.dynamic = true;
    endpoints.mesh.value = std::move(m);

    Surface s;
    s.shader = shader_;
    endpoints.surface.value = std::move(s);
}

#ifdef SYGALDREYE_PLUGIN
#include "eyeballs_node_abi.hpp"
EYEBALLS_EXPORT_NODE(RubberBandController)
#endif
