# WI-4: Migrate water_surface to LitShader

## Goal

`water_surface` contains ~40 lines of Blinn-Phong GLSL that duplicates what
`lit_shader` already provides. Refactor `water_surface` so the shading pass uses
`LitShader` (or the shared lighting uniforms / GLSL include) while keeping its
custom Gerstner-wave vertex displacement.

## Approach

The wave-displacement computation is the unique part and must remain a custom vertex
shader. The fragment shading (ambient + diffuse + specular + foam threshold) is
generic. Options:
1. Factor the Blinn-Phong GLSL into a `lighting.glsl` inline string header shared
   by both `water_surface` and `lit_shader`.
2. Or route `water_surface` through `LitShader` entirely, passing wave-displaced
   positions as a mesh update rather than a vertex shader.

Prefer option 1 (GLSL snippet) since option 2 would require CPU-side wave summation.

## Acceptance Criteria

- No copy of the Blinn-Phong algorithm remains in `water_surface`'s fragment shader
- `LitShader`'s fragment GLSL (or a shared inline snippet) is the single source of truth
- Visual output is unchanged (verified via snapshot comparison before/after)
- `sh/build.sh` passes
- `water_surface_snapshot` still produces a correct image

## Notes

`terrain_generator` already uses `LitShader` correctly — use it as the reference.
The `Light` and `Material` structs in `components/light/` and `components/material/`
are the data contract.
