// Copyright 2026 Travis West
#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <Eigen/Core>

#include "gpu_texture.hpp"        // GpuTexture
#include "sygaldry_endpoints.hpp"  // Span, Axis
#include "tri_mesh.hpp"            // TriMeshData

// Render-as-nodes payloads (planning/render_as_nodes.md). Two edge payloads
// flow shader-specific node → draw node: Surface (appearance) and Mesh
// (drawable geometry). The draw node is the render endpoint; render_region
// reads its resolved Surface+Mesh and is the ONLY place that says GL. There
// is deliberately NO DrawDesc: a draw is the node plus its wiring.

// CPU geometry, shared so producers regenerate while consumers hold a frame.
// (Also declared in eyeballs_node_abi.hpp; identical alias, legal redecl.)
using MeshPtr = std::shared_ptr<const TriMeshData>;

// The gather payload of a lifted mesh producer (forest route 1,
// conformability.md): N DISTINCT meshes — one tree per seed. Borrowed view
// onto the LiftGroup's mesh_gather, valid during the tick.
struct MeshArray {
    const MeshPtr* data = nullptr;
    int            count = 0;
};

// How geometry is assembled into primitives. GL-free; render_region maps it
// to GL_TRIANGLES/etc. so nodes never reference GL constants.
enum class Primitive { Triangles, TriangleStrip, Lines, LineStrip, Points };

// A per-instance vertex attribute: the shader's `in` name + an N-row Span
// (rows = instance count, cols = components). Bound at divisor 1 by the
// boundary. One row ⇒ a single (non-instanced) draw.
struct InstanceAttr {
    std::string name;
    Span        data;
};

// Vert+frag GLSL. The boundary compiles once and caches a GlProgram by this
// handle's identity. Held by the shader-specific node; never a GlProgram —
// GL stays in render_region.
struct ShaderData {
    std::string vert;
    std::string frag;
};
using Shader = std::shared_ptr<const ShaderData>;

// One uniform value — the subset of payloads a GLSL uniform can hold.
using UniformValue = std::variant<float, Eigen::Vector2f, Eigen::Vector3f,
                                  Eigen::Vector4f, Eigen::Matrix4f, GpuTexture>;
struct Uniform {
    std::string  name;
    UniformValue value;
};

// A CPU image (e.g. a glyph atlas) a Surface wants bound to a sampler. Nodes
// can't touch GL, so they hand render_region the pixels + a stable key; the
// boundary uploads once, caches by key, and binds it to the sampler. pixels
// must outlive the frame (a node-owned buffer or static data).
struct ImageTex {
    std::string          sampler;  // GLSL sampler2D uniform name
    const void*          key;      // stable identity for the upload cache
    const unsigned char* pixels;
    int                  width;
    int                  height;
    int                  channels;  // 3 = RGB, 4 = RGBA
};

// Appearance payload (NOT named Material: the existing Material struct is
// Blinn-Phong params, an input to the lit-shader node). Program + bound
// uniforms + pipeline state. Emitted by a shader-specific node.
struct Surface {
    Shader                shader;
    std::vector<Uniform>  uniforms;
    std::vector<ImageTex> images;  // CPU images render_region uploads + binds
    bool                 blend       = false;
    bool                 additive    = false;  // blend as SRC_ALPHA,ONE (glow) vs over
    bool                 depth_test  = true;
    bool                 depth_write = true;
    bool                 cull_back   = true;
    bool                 cull_front  = false;  // inside-out meshes (sky dome)
};

// Drawable payload: base geometry + per-instance attributes + primitive
// mode. Emitted by a shader-specific node; consumed by the draw node.
// (Kind "drawable" during the bridge — MeshPtr still owns kind "mesh" until
// raw-geometry edges retire in phase D.)
struct Mesh {
    MeshPtr                   geometry;
    std::vector<InstanceAttr> instances;  // empty ⇒ single, non-instanced draw
    Primitive                 mode = Primitive::Triangles;
    // dynamic ⇒ regenerated every frame (water, terrain, editor, text). The
    // boundary rebuilds it per frame instead of caching by pointer identity,
    // which is unsafe when a freed TriMeshData's address is reused.
    bool                      dynamic = false;
};
