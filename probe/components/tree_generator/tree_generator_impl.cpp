// Copyright 2025 Travis West
#include "tree_generator_impl.hpp"
#include <Eigen/Geometry>
#include <cmath>

static constexpr float k_pi = 3.14159265358979323846f;

static uint32_t lcg(uint32_t s) { return s * 1664525u + 1013904223u; }
static float lcg_f(uint32_t s)  { return static_cast<float>(s & 0xFFFFFF) / 16777216.0f; }

void emit_cylinder(TriMeshData& mesh,
                   Eigen::Vector3f const& base,
                   Eigen::Vector3f const& tip,
                   float r_bot, float r_top,
                   int slices,
                   Eigen::Vector4f const& color)
{
    Eigen::Vector3f axis = (tip - base).normalized();
    Eigen::Vector3f perp = std::abs(axis.x()) < 0.9f
        ? Eigen::Vector3f{1,0,0} : Eigen::Vector3f{0,1,0};
    Eigen::Vector3f u = axis.cross(perp).normalized();
    Eigen::Vector3f v = axis.cross(u);

    auto ring = [&](float r, Eigen::Vector3f const& center, int i) {
        float a = 2.0f * k_pi * static_cast<float>(i) / static_cast<float>(slices);
        return center + r * (std::cos(a) * u + std::sin(a) * v);
    };

    uint32_t base_idx = static_cast<uint32_t>(mesh.vertices.size());
    for (int i = 0; i < slices; ++i) {
        int j = (i + 1) % slices;
        Eigen::Vector3f p0 = ring(r_bot, base, i);
        Eigen::Vector3f p1 = ring(r_bot, base, j);
        Eigen::Vector3f p2 = ring(r_top, tip,  i);
        Eigen::Vector3f p3 = ring(r_top, tip,  j);
        Eigen::Vector3f n = (p1 - p0).cross(p2 - p0).normalized();
        for (auto* p : {&p0, &p1, &p2}) { mesh.vertices.push_back({*p, n, color}); }
        mesh.indices.push_back(base_idx++);
        mesh.indices.push_back(base_idx++);
        mesh.indices.push_back(base_idx++);
        Eigen::Vector3f n2 = (p2 - p1).cross(p3 - p1).normalized();
        for (auto* p : {&p1, &p3, &p2}) { mesh.vertices.push_back({*p, n2, color}); }
        mesh.indices.push_back(base_idx++);
        mesh.indices.push_back(base_idx++);
        mesh.indices.push_back(base_idx++);
    }
}

void recurse(TriMeshData& branches,
             std::vector<BillboardInstance>& leaves,
             Eigen::Vector3f const& base,
             Eigen::Vector3f const& dir,
             float length, float radius,
             int depth,
             TreeParams const& tp,
             BranchParams bp)
{
    Eigen::Vector3f tip = base + dir * length;
    emit_cylinder(branches, base, tip,
                  radius, radius * bp.radius_scale,
                  tp.cylinder_slices, tp.bark_color);

    if (depth >= bp.max_depth) {
        if (tp.enable_leaves)
            leaves.push_back({tip,
                              {tp.leaf_cluster_radius, tp.leaf_cluster_radius},
                              tp.leaf_color});
        return;
    }

    float child_len    = length * bp.length_scale;
    float child_radius = radius * bp.radius_scale;

    for (int i = 0; i < bp.branch_count; ++i) {
        bp.seed = lcg(bp.seed + static_cast<uint32_t>(i));
        float noise = (lcg_f(bp.seed) * 2.0f - 1.0f) * bp.angle_noise;
        float angle = bp.spread_angle + noise;

        uint32_t ax_seed = lcg(bp.seed + 7u);
        Eigen::Vector3f perp = std::abs(dir.x()) < 0.9f
            ? Eigen::Vector3f{1,0,0} : Eigen::Vector3f{0,1,0};
        perp += Eigen::Vector3f{lcg_f(ax_seed)*0.4f - 0.2f,
                                lcg_f(lcg(ax_seed))*0.4f - 0.2f,
                                lcg_f(lcg(lcg(ax_seed)))*0.4f - 0.2f};
        Eigen::Vector3f rot_axis = dir.cross(perp).normalized();
        Eigen::Vector3f child_dir =
            Eigen::AngleAxisf(angle, rot_axis) * dir;

        recurse(branches, leaves, tip, child_dir.normalized(),
                child_len, child_radius, depth + 1, tp, bp);
    }
}
