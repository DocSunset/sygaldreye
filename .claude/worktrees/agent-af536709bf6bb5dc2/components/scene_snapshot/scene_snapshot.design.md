# scene_snapshot

Renders one frame into an offscreen FBO and writes a PNG. Lets agents and developers preview a scene without a headset.

## Ports

**Inputs**
- `SnapshotParams` — resolution, projection/view matrices, time

**Outputs**
- PNG file at caller-specified path

**Sources**
- Current OpenGL ES context (caller must create via `host_gl_context` before calling)

**Destinations**
- None

**Temporal couplings**
- A live GL context must exist for the duration of `write_snapshot`

**Intended seams**
- `draw_fn` callback — caller injects any scene draw logic

## Requirements

- Create RGBA8 + DEPTH_COMPONENT24 FBO at the requested resolution
- Call `draw_fn` with projection and view matrices
- Read back pixels and flip rows (GL bottom-up → PNG top-down)
- Write PNG via `stb_image_write`; return false on any GL or IO error
- No persistent state; `write_snapshot` is a pure free function

## Allowed dependencies

- `host_gl_context` (caller owns; `scene_snapshot` does not create or destroy contexts)
- Eigen (matrix types)
- stb_image_write (single-header, vendored via Nix `pkgs.stb`)

## Owning package

`scene` (host tooling sub-package)
