// Copyright 2026 Travis West
#pragma once
#include <memory>
#include <string_view>
#include <vector>

#include "gl_program.hpp"
#include "sygaldry_endpoints.hpp"

// Bezier wires from an N×10 span: each row is {from.xyz, to.xyz, rgba}.
// First peel of the VrEditor monolith (vr_editor_decomposition.md): the
// editor publishes wire endpoints as data; this node owns how they look.
struct WireMeshNode {
    static consteval std::string_view name() { return "wire_mesh"; }
    static consteval std::string_view source_header() {
        return "components/wire_mesh/wire_mesh.hpp";
    }
    static consteval std::string_view source_cpp() { return "components/wire_mesh/wire_mesh.cpp"; }

    struct endpoints {
        in<Span> wires;  // N×10: from(3) to(3) rgba(4)
        out<DrawFn> render;
    } endpoints;
    void operator()(double);

private:
    std::vector<float> verts_;  // tessellated GL_LINES, pos3+rgba4 interleaved
    std::unique_ptr<GlProgram> prog_;
    unsigned vao_ = 0, vbo_ = 0;
};
