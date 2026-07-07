# light component

A pure-data struct representing a light source in the scene. Scene owns a list of lights and exposes them to the app layer for forwarding to the renderer. No GL or XR dependencies.

## Approach

- `light.hpp` declares:
  ```cpp
  enum class LightType { Directional };

  struct Light {
      LightType type      = LightType::Directional;
      Eigen::Vector3f direction = Eigen::Vector3f::UnitZ(); // unit vector, world space
      Eigen::Vector3f color     = Eigen::Vector3f::Ones();  // linear RGB
      float intensity           = 1.0F;
  };
  ```
- No `.cpp` needed.
- Scene exposes `std::span<const Light> lights() const` alongside the existing `cubes()`.
- Initial scene setup: one directional light, direction and intensity chosen to look good on the rotating cube.

## Design notes

- `type` enum reserved for future point/spot lights without changing the struct layout.
- Separate `color` and `intensity` (rather than a single pre-multiplied value) keeps authoring intuitive and supports HDR workflows later.
- Fields map directly to GLSL uniform struct layout.

## Acceptance

- No GL or XR headers included
- `light.design.md` present
- `scene` compiles with a `lights()` method returning at least one light
