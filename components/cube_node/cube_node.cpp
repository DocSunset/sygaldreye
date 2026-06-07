// Copyright 2025 Travis West
#include "cube_node.hpp"
#include <Eigen/Geometry>

void CubeNode::operator()(double /*time_s*/) {
    if (!initialized_) { mesh_.init(); initialized_ = true; }

    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
    model(0, 3) = inputs.pos_x.value;
    model(1, 3) = inputs.pos_y.value;
    model(2, 3) = inputs.pos_z.value;
    const float s = inputs.scale.value;
    model(0, 0) = s; model(1, 1) = s; model(2, 2) = s;

    Light light;
    if (inputs.light_intensity.value != 0.f || inputs.light_dir.value.squaredNorm() > 0.f) {
        light.direction = inputs.light_dir.value;
        light.color     = inputs.light_color.value;
        light.intensity = inputs.light_intensity.value;
    } else {
        light.direction = Eigen::Vector3f{0.f, -1.f, 0.f};
        light.color     = Eigen::Vector3f::Ones();
        light.intensity = 1.0f;
    }

    Material mat;
    const Eigen::Vector3f col{inputs.color_r.value, inputs.color_g.value, inputs.color_b.value};
    mat.diffuse  = col;
    mat.ambient  = col * 0.1f;
    const float r = inputs.roughness.value;
    mat.shininess = 2.f / (r * r + 1e-4f) - 1.f;

    outputs.render.value = [this, model, light, mat](const Eigen::Matrix4f& pv) {
        mesh_.begin_batch({&light, 1}, Eigen::Vector3f::Zero());
        mesh_.draw(pv * model, model, mat);
        CubeMesh::end_batch();
    };
}
