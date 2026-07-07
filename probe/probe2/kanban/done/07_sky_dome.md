# sky_dome component

A large inverted sphere rendered with a vertical color gradient (horizon to zenith) plus optional sun/moon disc and star field. Completes every outdoor and space environment; drawn first with depth write disabled so it sits behind all other geometry.

## Approach

- `sky_dome.hpp` declares:
  ```cpp
  struct SkyParams {
      Eigen::Vector4f horizon_color;
      Eigen::Vector4f zenith_color;
      float radius = 500.0f;          // large enough to encompass the scene
      // Sun / moon (optional; alpha=0 disables)
      Eigen::Vector3f body_dir;       // normalised direction toward sun or moon
      Eigen::Vector4f body_color;
      float           body_angular_radius; // radians; ~0.009 for sun/moon
      // Stars: seeded point cloud, only visible when sky is dark
      int  star_count  = 2000;
      bool enable_stars = false;
  };

  class SkyDome {
  public:
      static SkyDome create(SkyParams const&);
      void set_params(SkyParams const&);   // update colours each frame cheaply
      void draw(Eigen::Matrix4f const& vp) const; // no model matrix — centred on camera
  };
  ```
- Uses `sphere_geometry` for the dome mesh. Fragment shader computes gradient by vertex Y (passed as varying), blends in sun/moon disc via `smoothstep` on the angle to `body_dir`.
- Stars rendered as a separate instanced point-sprite pass using a pre-seeded random position set.
- Depth write disabled during draw; depth test disabled entirely (always behind scene).

## Acceptance

- Sky gradient visible in-headset with smooth colour transition from horizon to zenith
- Sun disc appears at the correct direction, round, with soft edge
- Star field renders as white points on a dark sky
- `sky_dome.design.md` present; `.cpp` files under 100 lines each

## Dependencies

- `sphere_geometry` (kanban/ready/01)
- `scene_time` (item 05) — for animated sky transitions
