// Copyright 2025 Travis West
#include "cube_node.hpp"
#include <Eigen/Geometry>

void CubeNode::operator()(double /*time_s*/) {
    if (!initialized_) { mesh_.init(); initialized_ = true; }

    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
    model(0, 3) = endpoints.pos_x.get();
    model(1, 3) = endpoints.pos_y.get();
    model(2, 3) = endpoints.pos_z.get();
    const float s = endpoints.scale.get();
    model(0, 0) = s; model(1, 1) = s; model(2, 2) = s;

    Light light;
    Eigen::Vector3f light_dir = endpoints.light_dir.get();
    float intensity = endpoints.light_intensity.get();
    if (intensity != 0.f || light_dir.squaredNorm() > 0.f) {
        light.direction = light_dir;
        light.color     = endpoints.light_color.get();
        light.intensity = intensity;
    } else {
        light.direction = Eigen::Vector3f{0.f, -1.f, 0.f};
        light.color     = Eigen::Vector3f::Ones();
        light.intensity = 1.0f;
    }

    Material mat;
    const Eigen::Vector3f col{endpoints.color_r.get(), endpoints.color_g.get(),
                              endpoints.color_b.get()};
    mat.diffuse  = col;
    mat.ambient  = col * 0.25f;  // 0.1 read as pure black against bright skies
    const float r = endpoints.roughness.get();
    mat.shininess = 2.f / (r * r + 1e-4f) - 1.f;

    endpoints.render.value = [this, model, light, mat](const Eigen::Matrix4f& pv) {
        mesh_.begin_batch({&light, 1}, Eigen::Vector3f::Zero());
        mesh_.draw(pv * model, model, mat);
        CubeMesh::end_batch();
    };
}
