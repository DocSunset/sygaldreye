# particle_system component

A fixed-capacity CPU-simulated particle pool drawn as instanced billboarded quads. Covers: fireflies, camp-fire sparks/embers, snowflakes, rain, dust motes, pollen, volcanic ash — any effect made of many small, short-lived things.

## Approach

- `particle_system.hpp` declares:
  ```cpp
  struct Particle {
      Eigen::Vector3f position;
      Eigen::Vector3f velocity;
      Eigen::Vector4f color;      // RGBA; fade alpha over lifetime
      float           size;       // world-space quad half-extent
      float           life;       // remaining life in seconds; particle is dead when ≤ 0
  };

  struct EmitterParams {
      Eigen::Vector3f origin;
      Eigen::Vector3f velocity_min, velocity_max; // per-axis random range
      Eigen::Vector4f color_start, color_end;     // lerped over lifetime
      float size_start, size_end;
      float lifetime_min, lifetime_max;
      float emit_rate;            // particles per second
  };

  class ParticleSystem {
  public:
      explicit ParticleSystem(int capacity);
      void set_emitter(EmitterParams const&);
      void update(float dt, Eigen::Vector3f gravity = {0,-9.8f,0});
      void draw(Eigen::Matrix4f const& vp, Eigen::Vector3f camera_right, Eigen::Vector3f camera_up) const;
  };
  ```
- Particles are stored in a fixed `std::array`-like pool (capacity set at construction, no allocation in `update`/`draw`). Dead particles are recycled in a ring.
- `draw()` builds a per-frame instance buffer (position + size + color) and submits one `glDrawArraysInstanced` call for all live particles, using a unit quad VAO.
- Quads are billboarded by passing `camera_right` and `camera_up` to the vertex shader.
- `particle_system.test.cpp`: verify particles die after their lifetime, that `update` with `dt=0` produces no motion, and that capacity is never exceeded.

## Acceptance

- A fire emitter at a fixed world point produces visible rising sparks in-headset
- No per-frame heap allocation in `update` or `draw`
- `particle_system.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `scene_time` (item 05) — `dt` sourced from frame time
- `rgba_shader` (kanban/ready/03) — or a dedicated billboard shader; both work
