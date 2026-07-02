// Copyright 2026 Travis West
#pragma once
#include <Eigen/Core>
#include <cstdint>
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
// Caching: geometry is keyed by TriMeshData* and validated by its globally
// unique version stamp (re-upload on mismatch, bind-only on match — the second
// eye never re-uploads). Programs are keyed by a hash of the GLSL source, so N
// instances of one shader compile once and survive graph swaps. Entries whose
// payload died (weak_ptr expired) or that go unused for kEvictAfterFrames are
// deleted, so caches never accumulate.
//
// Threading: enqueue/begin_frame are GL-free (graph/tick thread). issue() runs
// on the render thread with a current GL context. In both shells tick and
// render share a thread, so the queue needs no lock.
class RenderRegion {
public:
    static RenderRegion& instance();

    void begin_frame();                                      // render-head: clear queue, ++frame
    void enqueue(const Mesh& mesh, const Surface& surface);  // draw node, tick order
    // Graph swap: geometry/program caches are version/content keyed and need
    // nothing; only version-0 image textures (keyed by a node-owned pointer
    // whose address may be reused by the new graph) are dropped.
    void notify_graph_swap();
    // render thread: replay. view+proj separately so camera-facing shaders get
    // uCameraRight/uCameraUp (world-space camera basis from the view rows).
    // time_s feeds uTime for animated surfaces.
    void issue(const Eigen::Matrix4f& view, const Eigen::Matrix4f& proj, double time_s);

    // Test/diagnostic seam: cumulative cache activity + current cache sizes.
    struct Stats {
        std::uint64_t geometry_uploads = 0;
        std::uint64_t program_compiles = 0;
        std::uint64_t texture_uploads = 0;
    };
    const Stats& stats() const { return stats_; }
    std::size_t cached_geometries() const { return geometries_.size(); }
    std::size_t cached_programs() const { return programs_.size(); }

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
    GlGeometry* geometry_for(const Mesh& mesh);
    unsigned int texture_for(const ImageTex& img);
    void evict_stale();  // once per frame, on the GL thread (from issue)

    struct GeoEntry {
        GlGeometry geo;
        std::weak_ptr<const TriMeshData> alive;  // expired ⇒ evict promptly
        std::uint64_t version = 0;
        std::uint64_t last_used = 0;
        std::size_t vbytes = 0;
        std::size_t icount = 0;
    };
    struct ProgEntry {
        GlProgram prog;
        std::uint64_t last_used = 0;
    };
    struct TexEntry {
        unsigned int tex = 0;
        std::uint64_t version = 0;
        std::uint64_t last_used = 0;
        int width = 0, height = 0, channels = 0;
    };
    std::unordered_map<const TriMeshData*, GeoEntry> geometries_;
    std::unordered_map<std::uint64_t, ProgEntry> programs_;  // key: GLSL source hash
    std::unordered_map<const void*, TexEntry> image_textures_;

    // Per-instance attribute VBOs, keyed [queue index][attr index]. The queue
    // is built once per tick and issued per eye, so a slot uploaded this frame
    // (same Span) is bound without a second glBufferData.
    struct InstSlot {
        unsigned int vbo = 0;
        const float* data = nullptr;
        int rows = 0, cols = 0;
        std::uint64_t frame = 0;  // frame of last upload
    };
    std::vector<std::vector<InstSlot>> inst_slots_;

    std::uint64_t frame_ = 0;
    std::uint64_t last_evict_frame_ = 0;
    bool warned_instance_rows_ = false;
    Stats stats_;
};
