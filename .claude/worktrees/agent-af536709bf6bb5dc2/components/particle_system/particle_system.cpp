#include "particle_system.hpp"
#include "gl_program.hpp"
#include "particle_system_shader.hpp"
#include "log.hpp"
#include <algorithm>
#include <cstdlib>

#define TAG "particle_system"
#define LOGE(...) LOG_E(TAG, __VA_ARGS__)

static float rand_range(float lo, float hi) {
    return lo + (hi - lo) * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
}

ParticleSystem::ParticleSystem(int capacity)
    : pool_(static_cast<size_t>(capacity)), capacity_(capacity),
      inst_buf_(static_cast<size_t>(capacity) * 8) {
    create_gl_resources();
}

ParticleSystem::~ParticleSystem() { destroy_gl_resources(); }

ParticleSystem::ParticleSystem(ParticleSystem&&) noexcept = default;
ParticleSystem& ParticleSystem::operator=(ParticleSystem&&) noexcept = default;

void ParticleSystem::create_gl_resources() {
    auto result = GlProgram::build(particle_system_shader::VERT, particle_system_shader::FRAG);
    if (!result) { LOGE("shader build failed"); return; }
    prog_ = std::make_unique<GlProgram>(std::move(*result));
    loc_vp_    = prog_->uniform_location("uVP");
    loc_right_ = prog_->uniform_location("uRight");
    loc_up_    = prog_->uniform_location("uUp");

    static constexpr float kQuad[] = { -1.f,-1.f, 1.f,-1.f, -1.f,1.f, 1.f,1.f };

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &quad_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuad), kQuad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glGenBuffers(1, &inst_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, inst_vbo_);
    // 8 floats per instance: pos(3) + size(1) + color(4)
    glBufferData(GL_ARRAY_BUFFER, capacity_ * 8 * static_cast<int>(sizeof(float)),
                 nullptr, GL_DYNAMIC_DRAW);
    constexpr GLsizei stride = 8 * sizeof(float);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glVertexAttribDivisor(1, 1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(4 * sizeof(float)));
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);
}

void ParticleSystem::destroy_gl_resources() {
    if (vao_)      { glDeleteVertexArrays(1, &vao_);  vao_ = 0; }
    if (quad_vbo_) { glDeleteBuffers(1, &quad_vbo_);  quad_vbo_ = 0; }
    if (inst_vbo_) { glDeleteBuffers(1, &inst_vbo_);  inst_vbo_ = 0; }
}

void ParticleSystem::set_emitter(EmitterParams const& p) { params_ = p; }

void ParticleSystem::emit_one(float lifetime) {
    Particle& p = pool_[static_cast<size_t>(next_dead_ % capacity_)];
    p.position  = params_.origin;
    p.velocity  = { rand_range(params_.velocity_min.x(), params_.velocity_max.x()),
                    rand_range(params_.velocity_min.y(), params_.velocity_max.y()),
                    rand_range(params_.velocity_min.z(), params_.velocity_max.z()) };
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
            p.size  = params_.size_start  + t * (params_.size_end  - params_.size_start);
        }
    }
}

void ParticleSystem::draw(Eigen::Matrix4f const& vp,
                          Eigen::Vector3f camera_right,
                          Eigen::Vector3f camera_up) const {
    if (!prog_) return;
    int live = 0;
    for (auto const& p : pool_) {
        if (p.life <= 0.f) continue;
        float* d = inst_buf_.data() + static_cast<size_t>(live) * 8;
        d[0]=p.position.x(); d[1]=p.position.y(); d[2]=p.position.z();
        d[3]=p.size;
        d[4]=p.color.x(); d[5]=p.color.y(); d[6]=p.color.z(); d[7]=p.color.w();
        ++live;
    }
    if (live == 0) return;

    prog_->use();
    GlProgram::uniform(loc_vp_, vp);
    glUniform3fv(loc_right_, 1, camera_right.data());
    glUniform3fv(loc_up_,    1, camera_up.data());

    glBindBuffer(GL_ARRAY_BUFFER, inst_vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    live * 8 * static_cast<int>(sizeof(float)), inst_buf_.data());

    glBindVertexArray(vao_);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, live);
    glBindVertexArray(0);
}
