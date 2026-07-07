// Copyright 2026 Travis West
#pragma once
#include <string_view>

#include "eyeballs_node_abi.hpp"
#include "render_payloads.hpp"
#include "render_region.hpp"

// draw: the render endpoint — the `dac` of rendering. Takes a Mesh and a
// Surface and enqueues itself with render_region in tick order; render_region
// replays the queue per eye and is the only place that says GL. There is no
// DrawDesc: the draw is this node plus its wiring.
//
// seq_in/seq_out declare submission ORDER via the DAG — chain draws from a
// render_head and topological order becomes draw order. The enqueue itself is
// unconditional (the event sequences, it does not gate).
struct DrawNode {
    static consteval std::string_view name() { return "draw"; }
    static consteval std::string_view source_header() {
        return "components/render_region/render_region_nodes.hpp";
    }
    struct endpoints {
        ::in<Mesh>    mesh;
        ::in<Surface> surface;
        event_in      seq_in;
        event_out     seq_out;
    } endpoints;
    void operator()(double) {
        RenderRegion::instance().enqueue(endpoints.mesh.get(), endpoints.surface.get());
        endpoints.seq_out.triggered = endpoints.seq_in.triggered;
    }
};

// forest_draw: the mesh-array render boundary (conformability.md forest
// route 1). Consumes the MeshArray a lifted mesh producer gathers (N
// DISTINCT meshes) and a Surface, enqueuing one draw per mesh. The
// per-element loop is the RENDER STRATEGY at the boundary — the same place
// render_region replays per eye — not a hand-rolled lift: the N meshes were
// produced by the executor lifting the generator. Wired whole<MeshArray> so
// the array is consumed as a unit (never re-lifted).
struct ForestDrawNode {
    static consteval std::string_view name() { return "forest_draw"; }
    static consteval std::string_view source_header() {
        return "components/render_region/render_region_nodes.hpp";
    }
    struct endpoints {
        whole<MeshArray> meshes;
        ::in<Surface>    surface;
        event_in         seq_in;
        event_out        seq_out;
    } endpoints;
    void operator()(double) {
        MeshArray a = endpoints.meshes.get();
        Surface   s = endpoints.surface.get();
        for (int i = 0; i < a.count; ++i) {
            Mesh m;
            m.geometry = a.data[i];
            m.mode     = Primitive::Triangles;
            RenderRegion::instance().enqueue(m, s);
        }
        endpoints.seq_out.triggered = endpoints.seq_in.triggered;
    }
};

// render_head: clears render_region's queue at frame start and fires the
// frame-start event that draws chain from to declare order. Root of the chain.
struct RenderHeadNode {
    static consteval std::string_view name() { return "render_head"; }
    static consteval std::string_view source_header() {
        return "components/render_region/render_region_nodes.hpp";
    }
    struct endpoints {
        event_out frame;
    } endpoints;
    void operator()(double) {
        RenderRegion::instance().begin_frame();
        endpoints.frame.triggered = true;
    }
};
