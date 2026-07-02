// Copyright 2026 Travis West
// Queue + GPU caches. The per-frame replay lives in render_region_issue.cpp.
#include "render_region.hpp"

#include <GLES3/gl3.h>

#include <cstddef>

#include "tri_mesh.hpp"

namespace {
// Entries unused this long are deleted (a stale mesh from a param edit, a
// producer retired by a graph swap). ~4 s at 72 Hz — long enough that nothing
// live ever thrashes, short enough that leaks stay bounded.
constexpr std::uint64_t kEvictAfterFrames = 300;

std::uint64_t fnv1a(std::uint64_t h, const std::string& s) {
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ull;
    }
    return h;
}

// Build a GlGeometry from a TriMeshData (TriVertex layout pos@0/normal@12/
// color@24, uploaded raw).
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

RenderRegion& RenderRegion::instance() {
    static RenderRegion r;
    return r;
}

RenderRegion::RenderRegion() = default;

void RenderRegion::begin_frame() {
    queue_.clear();
    ++frame_;
}

void RenderRegion::enqueue(const Mesh& mesh, const Surface& surface) {
    queue_.push_back({mesh, surface});
}

void RenderRegion::notify_graph_swap() {
    // Version/content-keyed caches survive swaps (dead entries age out).
    // Version-0 image textures are "immutable pixels at a stable address" —
    // a retired node's buffer address may be reused by the new graph, so drop
    // them here (static atlases just re-upload once).
    for (auto it = image_textures_.begin(); it != image_textures_.end();) {
        if (it->second.version == 0) {
            glDeleteTextures(1, &it->second.tex);
            it = image_textures_.erase(it);
        } else {
            ++it;
        }
    }
}

GlProgram* RenderRegion::program_for(const ShaderData* s) {
    const std::uint64_t key = fnv1a(fnv1a(1469598103934665603ull, s->vert) * 31, s->frag);
    auto it = programs_.find(key);
    if (it == programs_.end()) {
        auto built = GlProgram::build(s->vert.c_str(), s->frag.c_str());
        if (!built) return nullptr;
        ++stats_.program_compiles;
        it = programs_.emplace(key, ProgEntry{std::move(*built), frame_}).first;
    }
    it->second.last_used = frame_;
    return &it->second.prog;
}

GlGeometry* RenderRegion::geometry_for(const Mesh& mesh) {
    const TriMeshData* m = mesh.geometry.get();
    GeoEntry& e = geometries_[m];
    if (e.geo.vao() == 0 || e.version != m->version) {
        const std::size_t vbytes = m->vertices.size() * sizeof(TriVertex);
        if (e.geo.vao() != 0 && e.vbytes == vbytes && e.icount == m->indices.size()) {
            // Same topology (fixed-grid animation): re-upload vertices only.
            const auto* fv = reinterpret_cast<const float*>(m->vertices.data());
            e.geo.update_verts({fv, m->vertices.size() * (sizeof(TriVertex) / sizeof(float))});
        } else {
            e.geo = build_geometry(m, mesh.dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
            e.vbytes = vbytes;
            e.icount = m->indices.size();
        }
        e.alive = mesh.geometry;
        e.version = m->version;
        ++stats_.geometry_uploads;
    }
    e.last_used = frame_;
    return &e.geo;
}

unsigned int RenderRegion::texture_for(const ImageTex& img) {
    TexEntry& e = image_textures_[img.key];
    if (e.tex == 0 || e.version != img.version || e.width != img.width ||
        e.height != img.height || e.channels != img.channels) {
        const bool fresh = e.tex == 0;
        if (fresh) glGenTextures(1, &e.tex);
        glBindTexture(GL_TEXTURE_2D, e.tex);
        const GLenum ext = (img.channels == 4) ? GL_RGBA : GL_RGB;
        const GLint in = (img.channels == 4) ? GL_RGBA8 : GL_RGB8;
        glTexImage2D(
            GL_TEXTURE_2D, 0, in, img.width, img.height, 0, ext, GL_UNSIGNED_BYTE, img.pixels);
        if (fresh) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        e.version = img.version;
        e.width = img.width;
        e.height = img.height;
        e.channels = img.channels;
        ++stats_.texture_uploads;
    }
    e.last_used = frame_;
    return e.tex;
}

void RenderRegion::evict_stale() {
    const auto stale = [&](std::uint64_t last_used) {
        return frame_ - last_used > kEvictAfterFrames;
    };
    for (auto it = geometries_.begin(); it != geometries_.end();) {
        if (it->second.alive.expired() || stale(it->second.last_used))
            it = geometries_.erase(it);  // ~GlGeometry deletes the GL objects
        else
            ++it;
    }
    for (auto it = programs_.begin(); it != programs_.end();) {
        if (stale(it->second.last_used))
            it = programs_.erase(it);  // ~GlProgram deletes the program
        else
            ++it;
    }
    for (auto it = image_textures_.begin(); it != image_textures_.end();) {
        if (stale(it->second.last_used)) {
            glDeleteTextures(1, &it->second.tex);
            it = image_textures_.erase(it);
        } else {
            ++it;
        }
    }
}
