# material component

A pure-data struct describing how a surface responds to light. Decoupled from both scene logic and GL — scene produces materials, the renderer consumes them. No GL or XR dependencies.

## Approach

- `material.hpp` declares:
  ```cpp
  struct Material {
      Eigen::Vector3f ambient  = {0.1F, 0.1F, 0.1F};
      Eigen::Vector3f diffuse  = {0.8F, 0.8F, 0.8F};
      Eigen::Vector3f specular = {0.5F, 0.5F, 0.5F};
      float shininess          = 32.0F;
  };
  ```
- No `.cpp` needed.
- Each `CubeInstance` gains a `Material material` field (default-initialised is fine for now).

## Design notes

- Per-material shininess uniform is the current target. Per-vertex or texture-driven variation is a future concern; when that time comes, the shader changes internally and this struct may be extended or partially replaced — the seam is already clear.
- Fields intentionally flat (no nested colour type) so they map directly to GLSL uniform struct members with no conversion.

## Acceptance

- No GL or XR headers included
- `material.design.md` present
- `CubeInstance` carries a `Material`
