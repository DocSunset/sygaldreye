#include "cylinder_mesh.hpp"

#include <cmath>
#include <Eigen/Geometry>
#include <numbers>

MeshPtr make_cylinder(int slices) {
    auto m = std::make_shared<TriMeshData>();
    const float step = (2.0F * std::numbers::pi_v<float>) / static_cast<float>(slices);
    const Eigen::Vector4f white{1.f, 1.f, 1.f, 1.f};

    // Side: 2*slices vertices (radial normals), slices*6 indices.
    for (int i = 0; i < slices; ++i) {
        const float theta = static_cast<float>(i) * step;
        const float c = std::cos(theta), s = std::sin(theta);
        const Eigen::Vector3f n{c, s, 0.f};
        m->vertices.push_back({{c, s, 0.5F}, n, white});
        m->vertices.push_back({{c, s, -0.5F}, n, white});
    }
    for (int i = 0; i < slices; ++i) {
        const auto t0 = uint32_t(2 * i);
        const auto b0 = uint32_t(2 * i + 1);
        const auto t1 = uint32_t(2 * ((i + 1) % slices));
        const auto b1 = uint32_t(2 * ((i + 1) % slices) + 1);
        m->indices.insert(m->indices.end(), {t0, b0, t1, t1, b0, b1});
    }

    // Caps: a centre + a rim per cap, axial normals.
    for (int cap = 0; cap < 2; ++cap) {
        const float nz = (cap == 0) ? 1.0F : -1.0F;
        const float z = (cap == 0) ? 0.5F : -0.5F;
        const Eigen::Vector3f axn{0.f, 0.f, nz};
        const auto ctr = uint32_t(m->vertices.size());
        m->vertices.push_back({{0.0F, 0.0F, z}, axn, white});
        const auto rim = uint32_t(m->vertices.size());
        for (int i = 0; i < slices; ++i) {
            const float theta = static_cast<float>(i) * step;
            m->vertices.push_back({{std::cos(theta), std::sin(theta), z}, axn, white});
        }
        for (int i = 0; i < slices; ++i) {
            const auto r0 = uint32_t(rim + i);
            const auto r1 = uint32_t(rim + (i + 1) % slices);
            if (cap == 0)
                m->indices.insert(m->indices.end(), {ctr, r0, r1});
            else
                m->indices.insert(m->indices.end(), {ctr, r1, r0});
        }
    }
    return m;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Eigen::Matrix4f cylinder_transform(Eigen::Vector3f point_a, Eigen::Vector3f point_b, float radius) {
    Eigen::Vector3f axis = point_b - point_a;
    float len = axis.norm();
    Eigen::Vector3f dir = axis / len;
    Eigen::Quaternionf rot = Eigen::Quaternionf::FromTwoVectors(Eigen::Vector3f::UnitZ(), dir);
    Eigen::Affine3f xf = Eigen::Translation3f(((point_a + point_b) * 0.5F)) * rot *
                         Eigen::Scaling(radius, radius, len);
    return xf.matrix();
}
