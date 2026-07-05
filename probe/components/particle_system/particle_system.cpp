#include "particle_system.hpp"

#include <algorithm>
#include <cstdlib>

#include "tri_mesh.hpp"

namespace {

float rand_range(float lo, float hi) {
    return lo + (hi - lo) * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
}

// Camera-facing quad, per-instance offset/size/color. uMVP + uCameraRight/Up
// injected by render_region.
constexpr const char* kVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;     // quad corner [-0.5,0.5]
in vec3  aOffset;                    // per-instance world position
in float aSize;
in vec4  aColor;
uniform mat4 uMVP;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;
out vec2 vUV;
out vec4 vColor;
void main() {
    vUV = aPos.xy + 0.5;
    vColor = aColor;
    vec3 world = aOffset + (aPos.x * aSize) * uCameraRight + (aPos.y * aSize) * uCameraUp;
    gl_Position = uMVP * vec4(world, 1.0);
}
)";
constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
in vec2 vUV;
in vec4 vColor;
out vec4 fragColor;
void main() {
    float d = length(vUV - 0.5) * 2.0;
    float a = smoothstep(1.0, 0.0, d);
    fragColor = vec4(vColor.rgb, vColor.a * a);
}
)";

MeshPtr make_quad() {
    auto m = std::make_shared<TriMeshData>();
    auto v = [](float x, float y) {
        TriVertex t;
        t.position = Eigen::Vector3f(x, y, 0.f);
        t.normal   = Eigen::Vector3f(0.f, 0.f, 1.f);
        t.color    = Eigen::Vector4f(1.f, 1.f, 1.f, 1.f);
        return t;
    };
    m->vertices = {v(-0.5f, -0.5f), v(0.5f, -0.5f), v(0.5f, 0.5f), v(-0.5f, 0.5f)};
    m->indices  = {0, 1, 2, 0, 2, 3};
    return m;
}

}  // namespace

ParticleSystem::ParticleSystem(int capacity)
    : pool_(static_cast<size_t>(capacity)),
      capacity_(capacity),
      positions_(static_cast<size_t>(capacity) * 3),
      sizes_(static_cast<size_t>(capacity)),
      colors_(static_cast<size_t>(capacity) * 4) {}

void ParticleSystem::set_emitter(EmitterParams const& p) { params_ = p; }

void ParticleSystem::operator()(double time_s) {
    if (!quad_) quad_ = make_quad();
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});

    params_.emit_rate    = endpoints.emit_rate.get();
    params_.lifetime_min = endpoints.lifetime_min.get();
    params_.lifetime_max = endpoints.lifetime_max.get();
    params_.size_start   = endpoints.size_start.get();
    params_.size_end     = endpoints.size_end.get();
    params_.origin       = {endpoints.emit_x.get(), endpoints.emit_y.get(), endpoints.emit_z.get()};
    float up = endpoints.vel_up.get(), sp = endpoints.vel_spread.get();
    params_.velocity_min = {-sp, up * 0.6f, -sp};
    params_.velocity_max = {sp, up, sp};
    params_.color_start  = {endpoints.r.get(), endpoints.g.get(), endpoints.b.get(), 1.f};
    params_.color_end    = {endpoints.r.get(), endpoints.g.get(), endpoints.b.get(), 0.f};
    float dt             = (prev_time_ < 0.0) ? 0.0f : static_cast<float>(time_s - prev_time_);
    prev_time_           = time_s;
    if (dt > 0.f) update(dt, {0.f, endpoints.gravity_y.get(), 0.f});

    // Pack live particles into per-instance attribute buffers.
    int live = 0;
    for (auto const& p : pool_) {
        if (p.life <= 0.f) continue;
        positions_[static_cast<size_t>(live) * 3 + 0] = p.position.x();
        positions_[static_cast<size_t>(live) * 3 + 1] = p.position.y();
        positions_[static_cast<size_t>(live) * 3 + 2] = p.position.z();
        sizes_[static_cast<size_t>(live)]             = p.size;
        colors_[static_cast<size_t>(live) * 4 + 0]    = p.color.x();
        colors_[static_cast<size_t>(live) * 4 + 1]    = p.color.y();
        colors_[static_cast<size_t>(live) * 4 + 2]    = p.color.z();
        colors_[static_cast<size_t>(live) * 4 + 3]    = p.color.w();
        ++live;
    }

    Mesh m;
    m.geometry = quad_;
    m.mode     = Primitive::Triangles;
    m.instances.push_back({"aOffset", Span{positions_.data(), live, 3, Axis::Item, Axis::Cell}});
    m.instances.push_back({"aSize", Span{sizes_.data(), live, 1, Axis::Item, Axis::Cell}});
    m.instances.push_back({"aColor", Span{colors_.data(), live, 4, Axis::Item, Axis::Cell}});
    endpoints.mesh.value = std::move(m);

    Surface s;
    s.shader      = shader_;
    s.blend       = true;
    s.depth_write = false;
    s.cull_back   = false;
    endpoints.surface.value = std::move(s);
}

void ParticleSystem::emit_one(float lifetime) {
    Particle& p = pool_[static_cast<size_t>(next_dead_ % capacity_)];
    p.position  = params_.origin;
    p.velocity  = {rand_range(params_.velocity_min.x(), params_.velocity_max.x()),
                   rand_range(params_.velocity_min.y(), params_.velocity_max.y()),
                   rand_range(params_.velocity_min.z(), params_.velocity_max.z())};
    p.color     = params_.color_start;
    p.size      = params_.size_start;
    p.life      = lifetime;
    ++next_dead_;
}

void ParticleSystem::update(float dt, Eigen::Vector3f gravity) {
    emit_accum_ += dt * params_.emit_rate;
    while (emit_accum_ >= 1.f) {
        float lt = rand_range(params_.lifetime_min, params_.lifetime_max);
        emit_one(lt);
        emit_accum_ -= 1.f;
    }
    for (auto& p : pool_) {
        if (p.life <= 0.f) continue;
        p.life -= dt;
        p.position += p.velocity * dt + 0.5f * gravity * dt * dt;
        p.velocity += gravity * dt;
        if (p.life > 0.f) {
            float t = 1.f - p.life / std::max(params_.lifetime_max, 1e-6f);
            p.color = params_.color_start + t * (params_.color_end - params_.color_start);
            p.size  = params_.size_start + t * (params_.size_end - params_.size_start);
        }
    }
}
