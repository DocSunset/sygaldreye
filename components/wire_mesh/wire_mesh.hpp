// Copyright 2026 Travis West
#pragma once
#include <memory>
#include <string_view>

#include "render_payloads.hpp"  // Surface, Mesh, Shader
#include "sygaldry_endpoints.hpp"

// Bezier wires from an N×10 span: each row is {from.xyz, to.xyz, rgba}.
// Editor publishes wire endpoints as data (vr_editor_decomposition.md); this
// node tessellates them into an unlit, per-vertex-colored line Mesh + Surface.
// GL lives in render_region; the geometry is one held TriMeshData refilled in
// place each frame and touch()ed so the boundary re-uploads it once.
struct WireMeshNode {
    static consteval std::string_view name() { return "wire_mesh"; }
    static consteval std::string_view source_header() {
        return "components/wire_mesh/wire_mesh.hpp";
    }
    static consteval std::string_view source_cpp() { return "components/wire_mesh/wire_mesh.cpp"; }

    struct endpoints {
        in<Span>      wires;  // N×10: from(3) to(3) rgba(4)
        out<Surface>  surface;
        out<Mesh>     mesh;
    } endpoints;
    void operator()(double);

   private:
    Shader shader_;
    std::shared_ptr<TriMeshData> data_;  // mutated in place + touch()ed
};
