// Copyright 2026 Travis West
#include "render_region.hpp"

#include <GLES3/gl3.h>

#include <cstddef>
#include <type_traits>
#include <variant>

#include "tri_mesh.hpp"

RenderRegion& RenderRegion::instance() {
    static RenderRegion r;
    return r;
}

RenderRegion::RenderRegion() = default;

void RenderRegion::begin_frame() {
    queue_.clear();
    dyn_idx_ = 0;  // dynamic slots persist; reused round-robin this frame
}

void RenderRegion::enqueue(const Mesh& mesh, const Surface& surface) {
    queue_.push_back({mesh, surface});
}

void RenderRegion::notify_graph_swap() {
    // Pointer-keyed caches: a retired node's address may be reused by a new
    // node. Drop them so program_for/geometry_for rebuild against live nodes.
    programs_.clear();
    geometries_.clear();
}

GlProgram* RenderRegion::program_for(const ShaderData* s) {
    auto it = programs_.find(s);
    if (it != programs_.end()) return &it->second;
    auto built = GlProgram::build(s->vert.c_str(), s->frag.c_str());
    if (!built) return nullptr;
    return &programs_.emplace(s, std::move(*built)).first->second;
}

namespace {
// Build a GlGeometry from a TriMeshData (TriVertex layout pos@0/normal@12/
// color@24, uploaded raw like TriMesh).
GlGeometry build_geometry(const TriMeshData* m, GLenum usage) {
    const auto* fv = reinterpret_cast<const float*>(m->vertices.data());
    std::span<const float> verts{fv, m->vertices.size() * (sizeof(TriVertex) / sizeof(float))};
    std::vector<AttribDesc> layout = {
        {0, 3, static_cast<GLsizei>(sizeof(TriVertex)), offsetof(TriVertex, position)},
        {1, 3, static_cast<GLsizei>(sizeof(TriVertex)), offsetof(TriVertex, normal)},
        {2, 4, static_cast<GLsizei>(sizeof(TriVertex)), offsetof(TriVertex, color)},
    };
    return GlGeometry::create(verts, m->indices, std::move(layout), usage);
}
}  // namespace

GlGeometry* RenderRegion::geometry_for(const Mesh& mesh) {
    const TriMeshData* m = mesh.geometry.get();
    if (mesh.dynamic) {  // not pointer-cached; reuse a per-frame slot
        const std::size_t slot = dyn_idx_++;
        if (slot >= dyn_geoms_.size()) dyn_geoms_.emplace_back();
        DynSlot& d = dyn_geoms_[slot];
        const std::size_t vbytes = m->vertices.size() * sizeof(TriVertex);
        if (d.geo.vao() != 0 && d.vbytes == vbytes && d.icount == m->indices.size()) {
            const auto* fv = reinterpret_cast<const float*>(m->vertices.data());
            d.geo.update_verts({fv, m->vertices.size() * (sizeof(TriVertex) / sizeof(float))});
        } else {
            d.geo = build_geometry(m, GL_DYNAMIC_DRAW);
            d.vbytes = vbytes;
            d.icount = m->indices.size();
        }
        return &d.geo;
    }
    auto it = geometries_.find(m);
    if (it != geometries_.end()) return &it->second;
    return &geometries_.emplace(m, build_geometry(m, GL_STATIC_DRAW)).first->second;
}

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
}  // namespace

void RenderRegion::issue(const Eigen::Matrix4f& view, const Eigen::Matrix4f& proj, double time_s) {
    const Eigen::Matrix4f view_proj = proj * view;
    // World-space camera basis = rows of the view rotation (R^T columns).
    const Eigen::Vector3f cam_right = view.block<1, 3>(0, 0).transpose();
    const Eigen::Vector3f cam_up = view.block<1, 3>(1, 0).transpose();
    // World-space camera position = -Rᵀt (view = [R|t] maps world→eye).
    const Eigen::Vector3f cam_pos = -view.block<3, 3>(0, 0).transpose() * view.block<3, 1>(0, 3);
    for (auto& it : queue_) {
        if (!it.mesh.geometry || !it.surface.shader) continue;
        GlProgram* prog = program_for(it.surface.shader.get());
        if (!prog) continue;
        prog->use();

        if (it.surface.blend) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, it.surface.additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glDisable(GL_BLEND);
        }
        if (it.surface.depth_test)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        glDepthMask(it.surface.depth_write ? GL_TRUE : GL_FALSE);
        if (it.surface.cull_front) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
        } else if (it.surface.cull_back) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        } else {
            glDisable(GL_CULL_FACE);
        }

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
            auto cit = image_textures_.find(img.key);
            unsigned int tex = 0;
            if (cit == image_textures_.end()) {
                glGenTextures(1, &tex);
                glBindTexture(GL_TEXTURE_2D, tex);
                const GLenum ext = (img.channels == 4) ? GL_RGBA : GL_RGB;
                const GLint in = (img.channels == 4) ? GL_RGBA8 : GL_RGB8;
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    in,
                    img.width,
                    img.height,
                    0,
                    ext,
                    GL_UNSIGNED_BYTE,
                    img.pixels);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                image_textures_[img.key] = tex;
            } else {
                tex = cit->second;
            }
            glActiveTexture(GL_TEXTURE0 + tex_unit);
            glBindTexture(GL_TEXTURE_2D, tex);
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
        // N = the (shared) row count; "excess rank → instanced draw".
        const GLsizei n = it.mesh.instances.front().data.rows;
        if (n <= 0) continue;
        glBindVertexArray(geo->vao());
        while (inst_vbos_.size() < it.mesh.instances.size()) {
            GLuint v = 0;
            glGenBuffers(1, &v);
            inst_vbos_.push_back(v);
        }
        std::vector<GLint> locs;
        locs.reserve(it.mesh.instances.size());
        for (std::size_t i = 0; i < it.mesh.instances.size(); ++i) {
            const InstanceAttr& ia = it.mesh.instances[i];
            GLint loc = prog->attrib_location(ia.name.c_str());
            locs.push_back(loc);
            if (loc < 0 || !ia.data.data) continue;
            glBindBuffer(GL_ARRAY_BUFFER, inst_vbos_[i]);
            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(ia.data.rows * ia.data.cols * sizeof(float)),
                ia.data.data,
                GL_DYNAMIC_DRAW);
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
