# tri_mesh

General-purpose GPU mesh for arbitrary triangle geometry with per-vertex position, normal, and RGBA color.

## Ports

**Inputs**
- `create(TriMeshData const&)` — uploads geometry to GPU; must be called on the GL thread with a current context
- `update(TriMeshData const&)` — re-uploads geometry; uses `glBufferSubData` when vertex count is unchanged, `glBufferData` otherwise
- `draw()` — issues a `glDrawElements` call; caller is responsible for binding a shader and setting uniforms

**Outputs**
- None

**Sources**
- Requires an active OpenGL ES 3.0+ context on the calling thread

**Destinations**
- Caller's framebuffer receives rasterized triangles

**Temporal couplings**
- `create` must precede `update` and `draw`
- Must be destroyed on the same GL thread it was created on

**Intended seams**
- `TriMeshData` is a plain struct; callers provide arbitrary geometry

## Requirements

- Interleaved vertex layout: position (vec3), normal (vec3), color (vec4); attribute locations 0/1/2
- `update()` avoids full reallocation when vertex count is unchanged
- Move-only RAII; destructor releases VAO, VBO, EBO
- Indices are `uint32_t` (drawn with `GL_UNSIGNED_INT`)

## Allowed dependencies

- `egl_context` (test only)

## Owning package

`render`
