// Copyright 2025 Travis West
#include "terrain_generator.hpp"
#include "noise.hpp"
#include "log.hpp"
#include <Eigen/Geometry>
#include <cmath>

#define TAG "terrain_generator"

namespace {

// Smooth blend between named color bands based on normalized height [0,1]
Eigen::Vector3f band_color(float t, std::array<TerrainBand, 5> const& bands) {
    t = std::clamp(t, 0.0f, 1.0f);
    for (int i = 0; i < 4; ++i) {
        if (t <= bands[static_cast<size_t>(i + 1)].threshold) {
            float lo   = (i == 0) ? 0.0f : bands[static_cast<size_t>(i)].threshold;
            float hi   = bands[static_cast<size_t>(i + 1)].threshold;
            float span = hi - lo;
            float frac = (span > 1e-5f) ? (t - lo) / span : 0.0f;
            // Smooth step
            frac = frac * frac * (3.0f - 2.0f * frac);
            return bands[static_cast<size_t>(i)].color * (1.0f - frac)
                 + bands[static_cast<size_t>(i + 1)].color * frac;
        }
    }
    return bands[4].color;
}

constexpr const char* const VERT = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec4 aColor;
uniform mat4 uMVP;
uniform mat4 uModel;
flat out vec3 vNormal;
flat out vec4 vColor;
out vec3 vFragPos;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vFragPos = vec3(uModel * vec4(aPos, 1.0));
    vNormal = normalize(mat3(transpose(inverse(uModel))) * aNormal);
    vColor = aColor;
}
)";

constexpr const char* const FRAG = R"(#version 300 es
precision mediump float;
flat in vec3 vNormal;
flat in vec4 vColor;
in vec3 vFragPos;
uniform vec3  uViewPos;
uniform vec3  uSunDir;
uniform vec3  uSunColor;
uniform float uSunIntensity;
uniform vec3  uMatAmbient;
uniform vec3  uMatDiffuse;
uniform vec3  uMatSpecular;
uniform float uMatShininess;
out vec4 fragColor;
void main() {
    vec3  norm     = normalize(vNormal);
    vec3  lightDir = normalize(-uSunDir);
    vec3  viewDir  = normalize(uViewPos - vFragPos);
    vec3  halfDir  = normalize(lightDir + viewDir);
    float diff     = max(dot(norm, lightDir), 0.0);
    float spec     = pow(max(dot(norm, halfDir), 0.0), uMatShininess);
    vec3  sun      = uSunColor * uSunIntensity;
    vec3  base     = vColor.rgb * uMatDiffuse;
    vec3  result   = base * uMatAmbient
                   + base * sun * diff
                   + uMatSpecular * sun * spec;
    fragColor = vec4(result, 1.0);
}
)";

} // namespace

TriMeshData generate_terrain(TerrainParams const& p) {
    int const nv = p.grid_w * p.grid_h;
    int const nf = (p.grid_w - 1) * (p.grid_h - 1) * 2;

    std::vector<float> heights(static_cast<size_t>(nv));
    float h_min =  1e9f, h_max = -1e9f;

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
                return { static_cast<float>(lx) * p.cell_size,
                         heights[static_cast<size_t>(idx)],
                         static_cast<float>(lz) * p.cell_size };
            };

            auto emit_face = [&](int a, int b, int c) {
                Eigen::Vector3f pa = pos(a), pb = pos(b), pc = pos(c);
                Eigen::Vector3f n  = (pb - pa).cross(pc - pa).normalized();
                float avg_h = (pa.y() + pb.y() + pc.y()) / 3.0f;
                Eigen::Vector3f col3 = band_color((avg_h - h_min) / h_range, p.bands);
                Eigen::Vector4f col{col3.x(), col3.y(), col3.z(), 1.0f};
                for (auto* p3 : {&pa, &pb, &pc}) {
                    mesh.vertices[static_cast<size_t>(vi)] = { *p3, n, col };
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

TerrainRenderer TerrainRenderer::create(TerrainParams const& p) {
    TerrainRenderer r;
    r.params_ = p;
    TriMeshData data = generate_terrain(p);
    r.mesh_ = TriMesh::create(data);

    auto prog = GlProgram::build(VERT, FRAG);
    if (!prog) { LOG_E(TAG, "shader build failed"); return r; }
    r.prog_         = std::make_unique<GlProgram>(std::move(*prog));
    r.mvp_loc_      = r.prog_->uniform_location("uMVP");
    r.model_loc_    = r.prog_->uniform_location("uModel");
    r.view_pos_loc_ = r.prog_->uniform_location("uViewPos");
    r.sun_dir_loc_  = r.prog_->uniform_location("uSunDir");
    r.sun_col_loc_  = r.prog_->uniform_location("uSunColor");
    r.sun_int_loc_  = r.prog_->uniform_location("uSunIntensity");
    r.mat_amb_loc_  = r.prog_->uniform_location("uMatAmbient");
    r.mat_dif_loc_  = r.prog_->uniform_location("uMatDiffuse");
    r.mat_spe_loc_  = r.prog_->uniform_location("uMatSpecular");
    r.mat_shi_loc_  = r.prog_->uniform_location("uMatShininess");
    return r;
}

void TerrainRenderer::operator()(double /*time_s*/) {
    bool dirty = (params_.height_scale   != endpoints.height_scale.get())
              || (params_.lacunarity     != endpoints.lacunarity.get())
              || (params_.gain           != endpoints.gain.get())
              || (params_.noise_offset_x != endpoints.noise_offset_x.get())
              || (params_.noise_offset_z != endpoints.noise_offset_z.get());
    params_.height_scale   = endpoints.height_scale.get();
    params_.lacunarity     = endpoints.lacunarity.get();
    params_.gain           = endpoints.gain.get();
    params_.noise_offset_x = endpoints.noise_offset_x.get();
    params_.noise_offset_z = endpoints.noise_offset_z.get();
    params_.sun.intensity  = endpoints.sun_intensity.get();
    if (dirty || !prog_) {  // !prog_: constructed off render thread (HTTP parse)
        auto saved = endpoints;
        *this = TerrainRenderer::create(params_);
        endpoints = saved;
    }
    endpoints.render.value = [this](const Eigen::Matrix4f& vp) {
        draw(vp, Eigen::Matrix4f::Identity(), Eigen::Vector3f::Zero());
    };
}

void TerrainRenderer::set_sun(Light const& sun) { params_.sun = sun; }

void TerrainRenderer::draw(Eigen::Matrix4f const& mvp,
                           Eigen::Matrix4f const& model,
                           Eigen::Vector3f const& view_pos) const {
    if (!prog_) return;
    prog_->use();
    GlProgram::uniform(mvp_loc_,   mvp);
    GlProgram::uniform(model_loc_, model);
    glUniform3fv(view_pos_loc_, 1, view_pos.data());
    Eigen::Vector3f sd = params_.sun.direction.normalized();
    glUniform3fv(sun_dir_loc_, 1, sd.data());
    glUniform3fv(sun_col_loc_, 1, params_.sun.color.data());
    glUniform1f (sun_int_loc_,    params_.sun.intensity);
    glUniform3fv(mat_amb_loc_, 1, params_.material.ambient.data());
    glUniform3fv(mat_dif_loc_, 1, params_.material.diffuse.data());
    glUniform3fv(mat_spe_loc_, 1, params_.material.specular.data());
    glUniform1f (mat_shi_loc_,    params_.material.shininess);
    mesh_.draw();
}
