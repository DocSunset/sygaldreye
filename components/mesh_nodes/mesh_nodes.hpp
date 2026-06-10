// Copyright 2026 Travis West
#pragma once
#include "eyeballs_node_abi.hpp"
#include "tri_mesh.hpp"
#include "gl_program.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <memory>
#include <string_view>

// CPU geometry through the graph: mesh_grid generates, mesh_displace reads
// a texture BACK from the GPU and moves vertices on the CPU, mesh_render
// uploads and draws. GPU → CPU → GPU, all over edges.

struct MeshGridNode {
    static consteval std::string_view name() { return "mesh_grid"; }
    static consteval std::string_view source_header() { return "components/mesh_nodes/mesh_nodes.hpp"; }
    struct inputs {
        slider<"cells",   "", float, fp(2.f),   fp(200.f), fp(64.f)>  cells;
        slider<"size",    "", float, fp(0.5f),  fp(100.f), fp(10.f)>  size;
    } inputs;
    struct outputs { port<"mesh", MeshPtr> mesh; } outputs;
    void operator()(double);
private:
    int   cells_ = -1;
    float size_  = -1.f;
    MeshPtr cached_;
};

struct MeshDisplaceNode {
    static consteval std::string_view name() { return "mesh_displace"; }
    static consteval std::string_view source_header() { return "components/mesh_nodes/mesh_nodes.hpp"; }
    struct inputs {
        port<"mesh",    MeshPtr>    mesh;
        port<"texture", GpuTexture> texture;   // RGBA8 readable (route float
                                               // textures through glsl_effect)
        slider<"amplitude", "", float, fp(-10.f), fp(10.f), fp(1.f)> amplitude;
        slider<"tint", "", float, fp(0.f), fp(1.f), fp(1.f)> tint; // vertex color from texture
    } inputs;
    struct outputs { port<"mesh", MeshPtr> mesh; } outputs;
    void operator()(double);
    ~MeshDisplaceNode();
private:
    GLuint fbo_ = 0;
    std::vector<unsigned char> pixels_;
    int tw_ = 0, th_ = 0;
};

struct MeshRenderNode {
    static consteval std::string_view name() { return "mesh_render"; }
    static consteval std::string_view source_header() { return "components/mesh_nodes/mesh_nodes.hpp"; }
    struct inputs {
        port<"mesh", MeshPtr> mesh;
        slider<"x", "", float, fp(-100.f), fp(100.f), fp(0.f)> x;
        slider<"y", "", float, fp(-100.f), fp(100.f), fp(0.f)> y;
        slider<"z", "", float, fp(-100.f), fp(100.f), fp(0.f)> z;
        port<"light_dir", Eigen::Vector3f> light_dir;
    } inputs;
    struct outputs { port<"render", DrawFn> render; } outputs;
    MeshRenderNode() { inputs.light_dir.value = {-0.4f, -0.8f, -0.3f}; }
    void operator()(double);
private:
    std::unique_ptr<GlProgram> prog_;
    TriMesh gpu_;
    const TriMeshData* uploaded_ = nullptr;
    MeshPtr held_;
};
