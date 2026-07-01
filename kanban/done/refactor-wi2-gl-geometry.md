# WI-2: GlGeometry Utility Component

## Goal

Extract the ~50-line VAO/VBO/EBO setup pattern that is replicated verbatim in 6 visual
nodes into a reusable `GlGeometry` component. The component wraps one VAO with 1–2 VBOs
and an optional EBO, with helpers for static and dynamic upload.

```cpp
struct AttributeDesc { GLuint index; GLint size; GLenum type; GLsizei stride; uintptr_t offset; };

class GlGeometry {
public:
    static GlGeometry create(std::span<const float> verts,
                             std::span<const uint32_t> indices,
                             std::vector<AttributeDesc> layout,
                             GLenum usage = GL_STATIC_DRAW);
    void update_verts(std::span<const float> verts);
    void draw_elements(GLenum mode, GLsizei count) const;
    void draw_arrays(GLenum mode, GLsizei count) const;
    ~GlGeometry();
    GlGeometry(GlGeometry&&) noexcept;
    GlGeometry& operator=(GlGeometry&&) noexcept;
    GlGeometry(const GlGeometry&) = delete;
    GlGeometry& operator=(const GlGeometry&) = delete;
};
```

## Affected Nodes

Migrate: `aurora`, `lissajous`, `water_surface`, `sky_dome`, `particle_system`
(chladni and reaction_diffusion already use TriMesh — leave them)

## Acceptance Criteria

- New component `components/gl_geometry/gl_geometry.hpp/.cpp/CMakeLists.txt`
- At least `aurora` and `lissajous` migrated (these are the clearest cases)
- `sh/build.sh` passes
- Snapshot binaries for migrated nodes still build and render correctly
- Unit test: `gl_geometry.test.cpp` verifies create/update/destroy without GL crash
  (requires host GL context — use HostGlContext)

## Notes

`water_surface` uses dynamic vertex upload every frame (Gerstner summation);
`GlGeometry::update_verts()` must support this. `particle_system` uses instancing
VBOs — handle as a second VBO or defer to a follow-up.
