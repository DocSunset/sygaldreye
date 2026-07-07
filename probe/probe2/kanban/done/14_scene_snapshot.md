# scene_snapshot component

Renders one frame of a scene into an offscreen FBO at a given resolution and camera transform, reads the pixels back, and writes a PNG file. The primitive that lets agents (and developers) preview what a scene looks like at a given time, position, and orientation without a headset.

## Approach

- `scene_snapshot.hpp` declares:
  ```cpp
  struct SnapshotParams {
      int width  = 1280;
      int height = 720;
      Eigen::Matrix4f projection; // standard perspective, caller constructs
      Eigen::Matrix4f view;       // world-to-camera
      float time_s = 0.0f;
  };

  // draw_fn receives (projection, view) and issues GL draw calls for the scene.
  // Returns false and logs on any error.
  bool write_snapshot(SnapshotParams const&,
                      std::function<void(Eigen::Matrix4f const& proj,
                                         Eigen::Matrix4f const& view)> const& draw_fn,
                      std::string const& png_path);
  ```
- Implementation:
  1. Create and bind an FBO with a colour renderbuffer (RGBA8) and a depth renderbuffer (DEPTH_COMPONENT24) at the requested resolution.
  2. Set viewport, clear, call `draw_fn`.
  3. `glReadPixels` → `std::vector<uint8_t>` (RGBA, bottom-up).
  4. Flip rows (OpenGL origin is bottom-left; PNG convention is top-left).
  5. Write PNG via `stb_image_write` (`stbi_write_png`). Pull `stb` via Nix (`pkgs.stb`) or vendor `stb_image_write.h` directly (single header, no build step).
  6. Delete FBO and renderbuffers.
- No persistent state — `write_snapshot` is a free function. The caller owns the GL context (created via `host_gl_context`).
- `scene_snapshot.test.cpp`: render a solid-colour scene (one `glClearColor` call, no geometry), write a PNG, read it back with `stb_image` and assert every pixel matches the clear colour.

## Acceptance

- A call with a trivial draw function produces a valid, correctly-sized PNG at `png_path`
- Pixel values in the test match the GL clear colour (within rounding)
- `write_snapshot` is usable from a standalone `main()` with ~10 lines of setup
- `scene_snapshot.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `host_gl_context` (item 13) — caller must have a live context before calling `write_snapshot`
