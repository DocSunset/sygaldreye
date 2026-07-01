# lit_shader component

A Blinn-Phong GLSL shader program for lit geometry. Used by all mesh types that carry normals (spheres, cylinders, cubes). Supports up to `MAX_LIGHTS` directional lights in a single pass.

## Approach

- `lit_shader.hpp` declares `LitShader` in the `render` package with:
  - `init()` — compile and link GLSL, cache uniform locations
  - `use()`
  - `set_mvp(const Eigen::Matrix4f&)`
  - `set_model(const Eigen::Matrix4f&)` — needed for normal-space transform
  - `set_view_pos(const Eigen::Vector3f&)` — camera world position, for specular
  - `set_lights(std::span<const Light>)` — copies up to `MAX_LIGHTS` entries; sets `uLightCount`
  - `set_material(const Material&)`

- Vertex shader (location layout):
  - `location=0`: position (vec3)
  - `location=1`: normal (vec3)
  - `location=2`: uv (vec2) — **reserved, unused for now**; attribute slot held open for texture mapping
  - Outputs: `vNormal` (world space, via normal matrix), `vFragPos` (world space), `vUV` (passed through)
  - Normal matrix computed as `mat3(transpose(inverse(uModel)))` in the vertex shader.

- Fragment shader:
  - Iterates `uLightCount` lights (capped to `MAX_LIGHTS = 8`)
  - Each light contributes: ambient + diffuse (Lambert) + specular (Blinn-Phong half-vector)
  - Final colour: accumulated contributions modulated by material ambient/diffuse/specular and light color × intensity
  - `uV` passed through but unused (ready for a `texture(uAlbedo, vUV)` substitution)

- `lit_shader.cpp` compiles and links GLSL, caches all uniform locations. `set_lights` maps `Light` fields (direction, color, intensity) onto the `uLights[i]` struct uniforms.

## Design notes

- `MAX_LIGHTS = 8` as a compile-time constant. Unused light slots are zeroed; the loop naturally contributes nothing for them.
- The `uModel` uniform (separate from MVP) is the seam for future normal-matrix variants — e.g. passing a precomputed normal matrix uniform instead of computing it in the shader, without changing the calling interface.
- UV slot at location=2 is the seam for texture-based material variation. When albedo textures arrive, the vertex data gains UVs, the shader samples `uAlbedo`, and `set_material` gains a texture handle — everything else stays the same.
- `set_lights` accepts `std::span<const Light>` (scene package type). `lit_shader.cpp` maps the fields manually; no scene header pulled into `lit_shader.hpp`.

## Acceptance

- Blinn-Phong highlight visible on a rotating cube with one directional light on-device
- `set_lights({})` (zero lights) renders the mesh black (ambient only from material, no light contribution) without crashing
- No GL errors
- `lit_shader.design.md` present; each `.cpp` under 100 lines of substantive code

## Dependencies

- `light` (item 02)
- `material` (item 03)
- `gl_program`
