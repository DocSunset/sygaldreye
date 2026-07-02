// Copyright 2026 Travis West
// The per-frame replay: bind program/state/uniforms/geometry and glDraw.
#include <GLES3/gl3.h>

#include <algorithm>
#include <cstdio>
#include <type_traits>
#include <variant>

#include "render_region.hpp"
#include "tri_mesh.hpp"

namespace {
GLenum gl_mode(Primitive p) {
    switch (p) {
        case Primitive::Triangles:
            return GL_TRIANGLES;
        case Primitive::TriangleStrip:
            return GL_TRIANGLE_STRIP;
        case Primitive::Lines:
            return GL_LINES;
        case Primitive::LineStrip:
            return GL_LINE_STRIP;
        case Primitive::Points:
            return GL_POINTS;
    }
    return GL_TRIANGLES;
}

// tex_unit is advanced for each texture bound, so multiple samplers coexist.
void set_uniform(const GlProgram& prog, const Uniform& u, int& tex_unit) {
    GLint loc = prog.uniform_location(u.name.c_str());
    if (loc < 0) return;  // shader doesn't declare it — silent no-op
    std::visit(
        [&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, float>)
                glUniform1f(loc, v);
            else if constexpr (std::is_same_v<T, Eigen::Vector2f>)
                glUniform2fv(loc, 1, v.data());
            else if constexpr (std::is_same_v<T, Eigen::Vector3f>)
                glUniform3fv(loc, 1, v.data());
            else if constexpr (std::is_same_v<T, Eigen::Vector4f>)
                glUniform4fv(loc, 1, v.data());
            else if constexpr (std::is_same_v<T, Eigen::Matrix4f>)
                glUniformMatrix4fv(loc, 1, GL_FALSE, v.data());
            else if constexpr (std::is_same_v<T, GpuTexture>) {
                glActiveTexture(GL_TEXTURE0 + tex_unit);
                glBindTexture(GL_TEXTURE_2D, v.id);
                glUniform1i(loc, tex_unit);
                ++tex_unit;
            }
        },
        u.value);
}

void apply_pipeline_state(const Surface& s) {
    if (s.blend) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, s.additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
    if (s.depth_test)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    glDepthMask(s.depth_write ? GL_TRUE : GL_FALSE);
    if (s.cull_front) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    } else if (s.cull_back) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }
}
}  // namespace

void RenderRegion::issue(const Eigen::Matrix4f& view, const Eigen::Matrix4f& proj, double time_s) {
    if (frame_ != last_evict_frame_) {  // once per frame, GL context current
        evict_stale();
        last_evict_frame_ = frame_;
    }
    const Eigen::Matrix4f view_proj = proj * view;
    // World-space camera basis = rows of the view rotation (R^T columns).
    const Eigen::Vector3f cam_right = view.block<1, 3>(0, 0).transpose();
    const Eigen::Vector3f cam_up = view.block<1, 3>(1, 0).transpose();
    // World-space camera position = -Rᵀt (view = [R|t] maps world→eye).
    const Eigen::Vector3f cam_pos = -view.block<3, 3>(0, 0).transpose() * view.block<3, 1>(0, 3);
    if (inst_slots_.size() < queue_.size()) inst_slots_.resize(queue_.size());
    for (std::size_t qi = 0; qi < queue_.size(); ++qi) {
        auto& it = queue_[qi];
        if (!it.mesh.geometry || !it.surface.shader) continue;
        GlProgram* prog = program_for(it.surface.shader.get());
        if (!prog) continue;
        prog->use();
        apply_pipeline_state(it.surface);

        GLint mvp = prog->uniform_location("uMVP");
        if (mvp >= 0) glUniformMatrix4fv(mvp, 1, GL_FALSE, view_proj.data());
        GLint cr = prog->uniform_location("uCameraRight");
        if (cr >= 0) glUniform3fv(cr, 1, cam_right.data());
        GLint cu = prog->uniform_location("uCameraUp");
        if (cu >= 0) glUniform3fv(cu, 1, cam_up.data());
        GLint vp_pos = prog->uniform_location("uViewPos");
        if (vp_pos >= 0) glUniform3fv(vp_pos, 1, cam_pos.data());
        GLint ut = prog->uniform_location("uTime");
        if (ut >= 0) glUniform1f(ut, static_cast<float>(time_s));
        int tex_unit = 0;
        for (const auto& img : it.surface.images) {
            GLint loc = prog->uniform_location(img.sampler.c_str());
            if (loc < 0 || !img.pixels) continue;
            glActiveTexture(GL_TEXTURE0 + tex_unit);
            glBindTexture(GL_TEXTURE_2D, texture_for(img));
            glUniform1i(loc, tex_unit);
            ++tex_unit;
        }
        for (const auto& u : it.surface.uniforms) set_uniform(*prog, u, tex_unit);

        GlGeometry* geo = geometry_for(it.mesh);
        const GLenum mode = gl_mode(it.mesh.mode);
        const auto icount = static_cast<GLsizei>(it.mesh.geometry->indices.size());
        const auto vcount = static_cast<GLsizei>(it.mesh.geometry->vertices.size());

        if (it.mesh.instances.empty()) {
            if (icount > 0)
                geo->draw_elements(mode, icount);
            else
                geo->draw_arrays(mode, vcount);
            continue;
        }

        // Instanced lift: per-instance attribute Spans bound at divisor 1.
        // N = the smallest row count across attributes (a mismatch means a
        // producer bug — warn once rather than read past the smaller VBO).
        GLsizei n = 0;
        for (const InstanceAttr& ia : it.mesh.instances) {
            if (!ia.data.data) continue;
            const auto rows = static_cast<GLsizei>(ia.data.rows);
            if (n != 0 && rows != n && !warned_instance_rows_) {
                std::fprintf(stderr, "render_region: instance attribute row counts differ "
                                     "(%d vs %d); drawing the minimum\n", n, rows);
                warned_instance_rows_ = true;
            }
            n = (n == 0) ? rows : std::min(n, rows);
        }
        if (n <= 0) continue;
        glBindVertexArray(geo->vao());
        auto& slots = inst_slots_[qi];
        if (slots.size() < it.mesh.instances.size()) slots.resize(it.mesh.instances.size());
        std::vector<GLint> locs;
        locs.reserve(it.mesh.instances.size());
        for (std::size_t i = 0; i < it.mesh.instances.size(); ++i) {
            const InstanceAttr& ia = it.mesh.instances[i];
            GLint loc = prog->attrib_location(ia.name.c_str());
            locs.push_back(loc);
            if (loc < 0 || !ia.data.data) continue;
            InstSlot& sl = slots[i];
            if (sl.vbo == 0) glGenBuffers(1, &sl.vbo);
            glBindBuffer(GL_ARRAY_BUFFER, sl.vbo);
            // The queue is identical for both eyes: skip the second upload.
            const bool uploaded = sl.frame == frame_ && sl.data == ia.data.data &&
                                  sl.rows == ia.data.rows && sl.cols == ia.data.cols;
            if (!uploaded) {
                glBufferData(
                    GL_ARRAY_BUFFER,
                    static_cast<GLsizeiptr>(ia.data.rows * ia.data.cols * sizeof(float)),
                    ia.data.data,
                    GL_DYNAMIC_DRAW);
                sl.frame = frame_;
                sl.data = ia.data.data;
                sl.rows = ia.data.rows;
                sl.cols = ia.data.cols;
            }
            glEnableVertexAttribArray(static_cast<GLuint>(loc));
            glVertexAttribPointer(
                static_cast<GLuint>(loc),
                ia.data.cols,
                GL_FLOAT,
                GL_FALSE,
                ia.data.cols * static_cast<GLsizei>(sizeof(float)),
                nullptr);
            glVertexAttribDivisor(static_cast<GLuint>(loc), 1);
        }
        if (icount > 0)
            glDrawElementsInstanced(mode, icount, GL_UNSIGNED_INT, nullptr, n);
        else
            glDrawArraysInstanced(mode, 0, vcount, n);
        for (GLint loc : locs)
            if (loc >= 0) {
                glDisableVertexAttribArray(static_cast<GLuint>(loc));
                glVertexAttribDivisor(static_cast<GLuint>(loc), 0);
            }
        glBindVertexArray(0);
    }
    // Leave GL in a clean default for the rest of the frame (glClear and the
    // next region's pass): depth writes on, blending off.
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
