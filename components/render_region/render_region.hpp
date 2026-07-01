// Copyright 2026 Travis West
#pragma once
#include <Eigen/Core>
#include <unordered_map>
#include <vector>

#include "gl_geometry.hpp"
#include "gl_program.hpp"
#include "render_payloads.hpp"

// The render boundary — the GL "device" of rendering (the audio_region of
// graphics). Draw nodes enqueue (Mesh, Surface) in tick order; the render-head
// node clears the queue at frame start; render_region owns ALL GPU resources
// and is the ONLY place that issues glDraw. Singleton, like AudioEngine.
//
// Threading: enqueue/begin_frame are GL-free (graph/tick thread). issue() runs
// on the render thread with a current GL context. In both shells tick and
// render share a thread, so the queue needs no lock.
class RenderRegion {
public:
    static RenderRegion& instance();

    void begin_frame();                                      // render-head: clear queue
    void enqueue(const Mesh& mesh, const Surface& surface);  // draw node, tick order
    // Graph swap: drop the pointer-keyed caches. A retired node's ShaderData /
    // TriMeshData address can be reused by a new node, which would otherwise
    // resolve to the freed node's stale program/geometry. Called from the shell
    // when the old graph (and its payloads) is destructed.
    void notify_graph_swap();
    // render thread: replay. view+proj separately so camera-facing shaders get
    // uCameraRight/uCameraUp (world-space camera basis from the view rows).
    // time_s feeds uTime for animated surfaces.
    void issue(const Eigen::Matrix4f& view, const Eigen::Matrix4f& proj, double time_s);

    RenderRegion(const RenderRegion&) = delete;
    RenderRegion& operator=(const RenderRegion&) = delete;

private:
    RenderRegion();

    struct Item {
        Mesh mesh;
        Surface surface;
    };
    std::vector<Item> queue_;

    GlProgram* program_for(const ShaderData* s);
    GlGeometry* geometry_for(const Mesh& mesh);  // cached if static, rebuilt if dynamic

    // Caches keyed by payload identity. Programs are stable (a node holds its
    // ShaderData for life). STATIC geometry (mesh generators) is likewise
    // stable, so pointer-keying is safe. DYNAMIC geometry is rebuilt per frame
    // into dyn_geoms_ (cleared at begin_frame) — never pointer-cached, because
    // a regenerated TriMeshData may land at a freed address.
    std::unordered_map<const ShaderData*, GlProgram> programs_;
    std::unordered_map<const TriMeshData*, GlGeometry> geometries_;
    std::unordered_map<const void*, unsigned int> image_textures_;  // CPU-image uploads

    // Dynamic geometry: persistent slots reused round-robin per frame (by draw
    // index). A slot whose vertex/index counts match re-uploads vertices only
    // (update_verts) instead of rebuilding GL objects — cheap for fixed-
    // topology animated meshes (chladni, editor, RD). begin_frame resets the
    // index; slots persist across frames.
    struct DynSlot {
        GlGeometry geo;
        std::size_t vbytes = 0;
        std::size_t icount = 0;
    };
    std::vector<DynSlot> dyn_geoms_;
    std::size_t dyn_idx_ = 0;

    // Reusable streaming VBOs for per-instance attributes — one per instance
    // attribute slot, grown on demand, refilled each instanced draw.
    std::vector<unsigned int> inst_vbos_;
};
