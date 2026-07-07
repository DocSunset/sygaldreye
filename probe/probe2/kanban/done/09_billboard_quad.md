# billboard_quad component

A single camera-facing quad mesh with per-instance position, size, and color, drawn via instanced rendering. The efficient substrate for dense foliage: individual grass blades, leaf clusters, flowers, shrubs, firefly glows, cloud puffs.

## Approach

- `billboard_quad.hpp` declares:
  ```cpp
  struct BillboardInstance {
      Eigen::Vector3f position;
      Eigen::Vector2f size;       // world-space half-extents (width, height)
      Eigen::Vector4f color;
  };

  class BillboardQuad {
  public:
      static BillboardQuad create(int max_instances);
      void set_instances(std::span<BillboardInstance const>);
      void draw(Eigen::Matrix4f const& vp,
                Eigen::Vector3f camera_right,
                Eigen::Vector3f camera_up) const;
  };
  ```
- One VAO with a static unit-quad VBO plus a dynamic per-instance VBO updated via `glBufferSubData`.
- Vertex shader reconstructs world position as `position + right * u * size.x + up * v * size.y`.
- Unlike `particle_system`, this is purely a rendering component — no simulation logic. It renders whatever instance list it is given.
- Optional: `axis_aligned` mode where quads face the camera horizontally only (good for grass blades).

## Acceptance

- 500 grass-blade instances covering a 10×10 m patch render in-headset with correct camera-facing orientation
- `set_instances` with zero instances produces no draw (no GL errors)
- `billboard_quad.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `rgba_shader` (kanban/ready/03) or a dedicated billboard shader
