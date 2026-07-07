// Copyright 2025 Travis West
#include "terrain_generator.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include <Eigen/Geometry>

#include "noise.hpp"

namespace {

// Smooth blend between named color bands based on normalized height [0,1].
Eigen::Vector3f band_color(float t, std::array<TerrainBand, 5> const& bands) {
    t = std::clamp(t, 0.0f, 1.0f);
    for (int i = 0; i < 4; ++i) {
        if (t <= bands[static_cast<size_t>(i + 1)].threshold) {
            float lo   = (i == 0) ? 0.0f : bands[static_cast<size_t>(i)].threshold;
            float hi   = bands[static_cast<size_t>(i + 1)].threshold;
            float span = hi - lo;
            float frac = (span > 1e-5f) ? (t - lo) / span : 0.0f;
            frac       = frac * frac * (3.0f - 2.0f * frac);  // smoothstep
            return bands[static_cast<size_t>(i)].color * (1.0f - frac)
                 + bands[static_cast<size_t>(i + 1)].color * frac;
        }
    }
    return bands[4].color;
}

}  // namespace

TriMeshData generate_terrain(TerrainParams const& p) {
    int const nv = p.grid_w * p.grid_h;
    int const nf = (p.grid_w - 1) * (p.grid_h - 1) * 2;

    std::vector<float> heights(static_cast<size_t>(nv));
    float h_min = 1e9f, h_max = -1e9f;

    for (int z = 0; z < p.grid_h; ++z) {
        for (int x = 0; x < p.grid_w; ++x) {
            float nx = (p.noise_offset_x + static_cast<float>(x)) / static_cast<float>(p.grid_w - 1);
            float nz = (p.noise_offset_z + static_cast<float>(z)) / static_cast<float>(p.grid_h - 1);
            float h  = noise::fbm(nx, nz, p.octaves, p.lacunarity, p.gain) * p.height_scale;
            heights[static_cast<size_t>(z * p.grid_w + x)] = h;
            h_min = std::min(h_min, h);
            h_max = std::max(h_max, h);
        }
    }

    float h_range = (h_max > h_min) ? (h_max - h_min) : 1.0f;

    TriMeshData mesh;
    mesh.vertices.resize(static_cast<size_t>(nf * 3));
    mesh.indices.resize(static_cast<size_t>(nf * 3));

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
                return {static_cast<float>(lx) * p.cell_size,
                        heights[static_cast<size_t>(idx)],
                        static_cast<float>(lz) * p.cell_size};
            };

            auto emit_face = [&](int a, int b, int c) {
                Eigen::Vector3f pa = pos(a), pb = pos(b), pc = pos(c);
                Eigen::Vector3f n     = (pb - pa).cross(pc - pa).normalized();
                float           avg_h = (pa.y() + pb.y() + pc.y()) / 3.0f;
                Eigen::Vector3f col3  = band_color((avg_h - h_min) / h_range, p.bands);
                Eigen::Vector4f col{col3.x(), col3.y(), col3.z(), 1.0f};
                for (auto* p3 : {&pa, &pb, &pc}) {
                    mesh.vertices[static_cast<size_t>(vi)] = {*p3, n, col};
                    mesh.indices[static_cast<size_t>(vi)]  = static_cast<uint32_t>(vi);
                    ++vi;
                }
            };

            emit_face(i00, i10, i01);
            emit_face(i10, i11, i01);
        }
    }

    return mesh;
}

void Terrain::operator()(double /*time_s*/) {
    bool dirty = !generated_ || params_.height_scale != endpoints.height_scale.get()
              || params_.lacunarity != endpoints.lacunarity.get()
              || params_.gain != endpoints.gain.get()
              || params_.noise_offset_x != endpoints.noise_offset_x.get()
              || params_.noise_offset_z != endpoints.noise_offset_z.get();
    if (dirty) {
        params_.height_scale   = endpoints.height_scale.get();
        params_.lacunarity     = endpoints.lacunarity.get();
        params_.gain           = endpoints.gain.get();
        params_.noise_offset_x = endpoints.noise_offset_x.get();
        params_.noise_offset_z = endpoints.noise_offset_z.get();
        mesh_      = std::make_shared<const TriMeshData>(generate_terrain(params_));
        generated_ = true;
    }
    endpoints.mesh.value = mesh_;
}
