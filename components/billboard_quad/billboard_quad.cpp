#include "billboard_quad.hpp"
#include "billboard_quad_shader.hpp"
#include "gl_program.hpp"
#include <android/log.h>
#include <utility>
#include <vector>

#define TAG "billboard_quad"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// 9 floats per instance: pos(3) + size(2) + color(4)
static constexpr int kFloatsPerInstance = 9;
static constexpr GLsizei kStride = kFloatsPerInstance * static_cast<GLsizei>(sizeof(float));

BillboardQuad BillboardQuad::create(int max_instances) {
    BillboardQuad bq;

    auto result = GlProgram::build(billboard_quad_shader::VERT, billboard_quad_shader::FRAG);
    if (!result) { LOGE("shader build failed"); return bq; }
    bq.max_instances_ = max_instances;
    bq.pack_buf_.resize(static_cast<size_t>(max_instances) * kFloatsPerInstance);
    bq.prog_ = std::make_unique<GlProgram>(std::move(*result));
    bq.loc_vp_    = bq.prog_->uniform_location("uVP");
    bq.loc_right_ = bq.prog_->uniform_location("uRight");
    bq.loc_up_    = bq.prog_->uniform_location("uUp");

    static constexpr float kQuad[] = { -1.f,-1.f, 1.f,-1.f, -1.f,1.f, 1.f,1.f };

    glGenVertexArrays(1, &bq.vao_);
    glBindVertexArray(bq.vao_);

    glGenBuffers(1, &bq.quad_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, bq.quad_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuad), kQuad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glGenBuffers(1, &bq.inst_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, bq.inst_vbo_);
    glBufferData(GL_ARRAY_BUFFER, max_instances * kFloatsPerInstance * static_cast<int>(sizeof(float)),
                 nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, kStride, reinterpret_cast<void*>(0));
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, kStride, reinterpret_cast<void*>(3 * sizeof(float)));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, kStride, reinterpret_cast<void*>(5 * sizeof(float)));
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);
    return bq;
}

BillboardQuad::~BillboardQuad() {
    if (vao_)      { glDeleteVertexArrays(1, &vao_);  }
    if (quad_vbo_) { glDeleteBuffers(1, &quad_vbo_); }
    if (inst_vbo_) { glDeleteBuffers(1, &inst_vbo_); }
}

BillboardQuad::BillboardQuad(BillboardQuad&&) noexcept = default;
BillboardQuad& BillboardQuad::operator=(BillboardQuad&&) noexcept = default;

void BillboardQuad::set_instances(std::span<BillboardInstance const> instances) {
    instance_count_ = static_cast<int>(instances.size());
    if (instance_count_ == 0) return;

    float* d = pack_buf_.data();
    for (auto const& inst : instances) {
        *d++ = inst.position.x(); *d++ = inst.position.y(); *d++ = inst.position.z();
        *d++ = inst.size.x();     *d++ = inst.size.y();
        *d++ = inst.color.x();    *d++ = inst.color.y();
        *d++ = inst.color.z();    *d++ = inst.color.w();
    }

    glBindBuffer(GL_ARRAY_BUFFER, inst_vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    instance_count_ * kFloatsPerInstance * static_cast<int>(sizeof(float)),
                    pack_buf_.data());
}

void BillboardQuad::draw(Eigen::Matrix4f const& vp,
                         Eigen::Vector3f camera_right,
                         Eigen::Vector3f camera_up) const {
    if (!prog_ || instance_count_ == 0) return;

    prog_->use();
    GlProgram::uniform(loc_vp_, vp);
    glUniform3fv(loc_right_, 1, camera_right.data());
    glUniform3fv(loc_up_,    1, camera_up.data());

    glBindVertexArray(vao_);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, instance_count_);
    glBindVertexArray(0);
}
