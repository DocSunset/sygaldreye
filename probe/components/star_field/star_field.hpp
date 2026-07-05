// Copyright 2025 Travis West
#pragma once
#include <memory>
#include <string_view>

#include "render_payloads.hpp"  // Surface, Mesh, Shader
#include "sygaldry_endpoints.hpp"

// star_field: procedural points (positions from gl_VertexID in the shader),
// fading in as the sun drops. Emits a points Mesh (N dummy vertices — the
// shader ignores them) + a blended Surface. GL lives in render_region.
class StarField {
   public:
    static consteval std::string_view name() { return "star_field"; }
    static consteval std::string_view source_header() {
        return "components/star_field/star_field.hpp";
    }
    static consteval std::string_view source_cpp() { return "components/star_field/star_field.cpp"; }

    struct endpoints {
        normalled_in<float, fp(-1.f), fp(1.f), fp(0.5f)> sun_elevation;
        normalled_in<float, fp(0.f), fp(5000.f), fp(2000.f)> star_count;
        normalled_in<float, fp(10.f), fp(2000.f), fp(500.f)> radius;
        ::out<Surface> surface;
        ::out<Mesh>    mesh;
    } endpoints;

    void operator()(double);

   private:
    MeshPtr points_;
    int     count_ = -1;
    Shader  shader_;
};
