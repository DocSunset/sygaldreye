// Copyright 2025 Travis West
#include "terrain_generator.hpp"
#include "noise.hpp"
#include <Eigen/Geometry>
#include <cmath>

static Eigen::Vector4f band_color(float t, std::array<Eigen::Vector4f, 4> const& bands) {
    t = std::clamp(t, 0.0f, 1.0f);
    float scaled = t * 3.0f;
    int   lo     = static_cast<int>(scaled);
    if (lo >= 3) return bands[3];
    float frac = scaled - static_cast<float>(lo);
    return bands[lo] * (1.0f - frac) + bands[lo + 1] * frac;
}

TriMeshData generate_terrain(TerrainParams const& p) {
    int const nv = p.grid_w * p.grid_h;
    int const nf = (p.grid_w - 1) * (p.grid_h - 1) * 2;

    std::vector<float> heights(nv);
    float h_min =  1e9f, h_max = -1e9f;

    for (int z = 0; z < p.grid_h; ++z) {
        for (int x = 0; x < p.grid_w; ++x) {
            float nx = (p.noise_offset_x + static_cast<float>(x)) / static_cast<float>(p.grid_w - 1);
            float nz = (p.noise_offset_z + static_cast<float>(z)) / static_cast<float>(p.grid_h - 1);
            float h  = noise::fbm(nx, nz, p.octaves, p.lacunarity, p.gain) * p.height_scale;
            heights[z * p.grid_w + x] = h;
            h_min = std::min(h_min, h);
            h_max = std::max(h_max, h);
        }
    }

    float h_range = (h_max > h_min) ? (h_max - h_min) : 1.0f;

    TriMeshData mesh;
    mesh.vertices.resize(nf * 3);
    mesh.indices.resize(nf * 3);

    int vi = 0;
    for (int z = 0; z < p.grid_h - 1; ++z) {
        for (int x = 0; x < p.grid_w - 1; ++x) {
            int i00 = z * p.grid_w + x;
            int i10 = i00 + 1;
            int i01 = i00 + p.grid_w;
            int i11 = i01 + 1;

            auto pos = [&](int idx) -> Eigen::Vector3f {
                int lx = idx % p.grid_w;
                int lz = idx / p.grid_w;
                return { static_cast<float>(lx) * p.cell_size,
                         heights[idx],
                         static_cast<float>(lz) * p.cell_size };
            };

            auto emit_face = [&](int a, int b, int c) {
                Eigen::Vector3f pa = pos(a), pb = pos(b), pc = pos(c);
                Eigen::Vector3f n  = (pb - pa).cross(pc - pa).normalized();
                float avg_h = (pa.y() + pb.y() + pc.y()) / 3.0f;
                Eigen::Vector4f col = band_color((avg_h - h_min) / h_range, p.band_colors);
                for (auto* p3 : {&pa, &pb, &pc}) {
                    mesh.vertices[vi] = { *p3, n, col };
                    mesh.indices[vi]  = static_cast<uint32_t>(vi);
                    ++vi;
                }
            };

            emit_face(i00, i10, i01);
            emit_face(i10, i11, i01);
        }
    }

    return mesh;
}
