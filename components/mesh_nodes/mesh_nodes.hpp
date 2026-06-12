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
    struct endpoints {
        normalled_in<float, fp(2.f),  fp(200.f), fp(64.f)> cells;
        normalled_in<float, fp(0.5f), fp(100.f), fp(10.f)> size;
        out<MeshPtr> mesh;
    } endpoints;
    void operator()(double);
private:
    int   cells_ = -1;
    float size_  = -1.f;
    MeshPtr cached_;
};

struct MeshDisplaceNode {
    static consteval std::string_view name() { return "mesh_displace"; }
    static consteval std::string_view source_header() { return "components/mesh_nodes/mesh_nodes.hpp"; }
    struct endpoints {
        in<MeshPtr>    mesh;
        in<GpuTexture> texture;   // RGBA8 readable (route float
                                  // textures through glsl_effect)
        normalled_in<float, fp(-10.f), fp(10.f), fp(1.f)> amplitude;
        normalled_in<float, fp(0.f), fp(1.f), fp(1.f)> tint; // vertex color from texture
        out<MeshPtr> mesh_out;    // processor convention: in mesh, out mesh_out
    } endpoints;
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
    struct endpoints {
        in<MeshPtr> mesh;
        normalled_in<float, fp(-100.f), fp(100.f), fp(0.f)> x, y, z;
        normalled_in<Eigen::Vector3f> light_dir;
        out<DrawFn> render;
    } endpoints;
    MeshRenderNode() { endpoints.light_dir.fallback = {-0.4f, -0.8f, -0.3f}; }
    void operator()(double);
private:
    std::unique_ptr<GlProgram> prog_;
    TriMesh gpu_;
    const TriMeshData* uploaded_ = nullptr;
    MeshPtr held_;
};

// ── generators ──────────────────────────────────────────────────────────────
struct MeshSphereNode {
    static consteval std::string_view name() { return "mesh_sphere"; }
    struct endpoints {
        normalled_in<float, fp(0.05f), fp(50.f),  fp(1.f)>  radius;
        normalled_in<float, fp(4.f),   fp(128.f), fp(32.f)> segments;
        out<MeshPtr> mesh;
    } endpoints;
    void operator()(double);
private:
    float radius_ = -1.f; int segs_ = -1;
    MeshPtr cached_;
};

struct MeshBoxNode {
    static consteval std::string_view name() { return "mesh_box"; }
    struct endpoints {
        normalled_in<float, fp(0.05f), fp(50.f), fp(1.f)> sx, sy, sz;
        out<MeshPtr> mesh;
    } endpoints;
    void operator()(double);
private:
    Eigen::Vector3f size_{-1.f, -1.f, -1.f};
    MeshPtr cached_;
};

struct MeshCylinderNode {
    static consteval std::string_view name() { return "mesh_cylinder"; }
    struct endpoints {
        normalled_in<float, fp(0.05f), fp(50.f),  fp(0.5f)> radius;
        normalled_in<float, fp(0.05f), fp(50.f),  fp(2.f)>  height;
        normalled_in<float, fp(3.f),   fp(128.f), fp(24.f)> segments;
        out<MeshPtr> mesh;
    } endpoints;
    void operator()(double);
private:
    float radius_ = -1.f, height_ = -1.f; int segs_ = -1;
    MeshPtr cached_;
};

// ── deformers ───────────────────────────────────────────────────────────────
// Sine ripple along vertex normals; phase travels with time.
struct MeshRippleNode {
    static consteval std::string_view name() { return "mesh_ripple"; }
    struct endpoints {
        in<MeshPtr> mesh;
        normalled_in<float, fp(-5.f), fp(5.f),  fp(0.2f)> amplitude;
        normalled_in<float, fp(0.1f), fp(20.f), fp(3.f)>  freq;
        normalled_in<float, fp(-10.f),fp(10.f), fp(1.f)>  speed;
        out<MeshPtr> mesh_out;
    } endpoints;
    void operator()(double);
};

// Twist around Y proportional to height.
struct MeshTwistNode {
    static consteval std::string_view name() { return "mesh_twist"; }
    struct endpoints {
        in<MeshPtr> mesh;
        normalled_in<float, fp(-6.2832f), fp(6.2832f), fp(1.f)> angle;
        out<MeshPtr> mesh_out;
    } endpoints;
    void operator()(double);
};

// Bake a mat4 into the vertices (positions + rotated normals).
struct MeshTransformNode {
    static consteval std::string_view name() { return "mesh_transform"; }
    struct endpoints {
        in<MeshPtr>         mesh;
        in<Eigen::Matrix4f> matrix;   // unwired reads Identity
        out<MeshPtr> mesh_out;
    } endpoints;
    void operator()(double);
};

// ── span era (conformability.md): lists as values ───────────────────────────

// N pseudo-random positions scattered in a disc — forest fodder. The
// first span producer: positions leave as an N×3 value.
struct ScatterNode {
    static consteval std::string_view name() { return "scatter"; }
    struct endpoints {
        normalled_in<float, fp(1.f),   fp(2000.f), fp(60.f)>  count;
        normalled_in<float, fp(0.5f),  fp(200.f),  fp(25.f)>  radius;
        normalled_in<float, fp(0.f),   fp(100.f),  fp(1.f)>   seed;
        normalled_in<float, fp(-10.f), fp(10.f),   fp(0.f)>   y;
        out<Span> positions;
    } endpoints;
    void operator()(double);
private:
    std::vector<float> pts_;
    int   n_ = -1;
    float r_ = -1.f, s_ = -1.f, y_ = -1e9f;
};

// One mesh drawn at every row of an N×3 span — the ratified forest
// acceptance, "lift the draw" route. Manual lifting until the
// conformability executor takes over.
struct MeshInstancesNode {
    static consteval std::string_view name() { return "mesh_instances"; }
    struct endpoints {
        in<MeshPtr> mesh;
        in<Span>    positions;
        normalled_in<Eigen::Vector3f> light_dir;
        out<DrawFn> render;
    } endpoints;
    MeshInstancesNode() { endpoints.light_dir.fallback = {0.4f, 0.8f, 0.3f}; }
    void operator()(double);
private:
    MeshPtr held_;
    std::vector<float> held_pts_;   // copy: draw runs after the tick
    std::unique_ptr<GlProgram> prog_;
    TriMesh gpu_;
    const TriMeshData* uploaded_ = nullptr;
};
