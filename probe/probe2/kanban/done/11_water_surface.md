# water_surface component

A flat grid mesh whose vertex heights are animated each frame by a sum of noise-driven sine waves, producing a plausible low-poly river, lake, or ocean surface. Uses `TriMesh::update()` to re-upload geometry each frame.

## Approach

- `water_surface.hpp` declares:
  ```cpp
  struct WaterParams {
      int   grid_w        = 32;
      int   grid_h        = 32;
      float cell_size     = 1.0f;
      float wave_height   = 0.08f;  // amplitude in metres
      float wave_speed    = 0.6f;
      Eigen::Vector4f shallow_color = {0.1f, 0.5f, 0.7f, 0.85f};
      Eigen::Vector4f deep_color    = {0.03f, 0.2f, 0.45f, 0.9f};
  };

  class WaterSurface {
  public:
      static WaterSurface create(WaterParams const&);
      void update(float time_s);  // recomputes heights, re-uploads mesh
      void draw(Eigen::Matrix4f const& mvp) const;
  };
  ```
- Height at each vertex: `h = wave_height * (sin(x * f1 + time * s1) + sin(z * f2 + time * s2)) * 0.5` — two crossed sine waves give a simple ripple without needing noise on the hot path.
- Normals recomputed per-face after height update; color lerped between `shallow_color` and `deep_color` by local wave crest height.
- Alpha < 1 requires `GL_BLEND`; callers enable/disable.
- `water_surface.test.cpp`: verify `update` at `time=0` and `time=1` produce different heights; verify normals remain unit length.

## Acceptance

- A water patch animates visibly in-headset — rippling surface, colour variation
- Semi-transparency shows geometry beneath the water
- `water_surface.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `tri_mesh` (item 03)
- `flat_shader` (item 02)
- `scene_time` (item 05)
