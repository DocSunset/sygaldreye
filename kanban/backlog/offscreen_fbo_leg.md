# Offscreen rendering on render_region FBO passes (the "FBO leg")

Revive the offscreen render path, now that `DrawFn` is deleted and GL lives
only in `render_region`. Deferred out of the lifting/editor/DrawFn arc
(2026-06-16); the nodes were **parked** (sources kept, dropped from the
build + both shells' registration) because they carried `in/out<DrawFn>`
and would not compile after the DrawFn teardown (ABI v8).

## Parked nodes to revive (currently out of build)
- `components/render_nodes/` — `RenderTargetNode` (`in<DrawFn> draw` →
  `out<GpuTexture> texture`) and `TextureViewNode` (`in<GpuTexture>` →
  `out<DrawFn> render`). The FBO offscreen target + an on-screen blit.
- `components/rd_renderer/` — reaction-diffusion visualizer (`out<DrawFn>`),
  samples `rd_gpu`'s texture. Has `rd_renderer.test.cpp` (was the one host
  binary dropped, 43→42).
- `components/glsl_effect/` — full-screen shader effect; `in<GpuTexture>` →
  `out<GpuTexture>` (parked because its input is produced only by
  `render_target`). Owns its own GL program today.
- `components/rd_gpu/` — STAYS as-is (resource-holder owning ping-pong FBOs,
  emits `GpuTexture`, never used `DrawFn`). Reference for "a node may own
  GPU resources when it IS the device for that resource."

Each is annotated `// parked: offscreen leg` and commented out of root
`CMakeLists.txt` + removed from `host_app.cpp` / `app.cpp` registration.

## Design — offscreen as a render_region capability, not node-owned GL
The boundary owns all GL (the precedent `render_region` already set). An
offscreen pass is a **second draw queue with an FBO target** whose color
attachment becomes a `GpuTexture` edge payload:

- **`render_target` = a `render_head` with an FBO target.** It opens a
  named sub-pass: `render_region.begin_pass(target_id, w, h, clear)` clears
  + binds an owned FBO; draws chained from it (via the same `seq` event
  chain) enqueue into THAT pass instead of the main queue; at issue time
  the boundary replays the sub-pass into the FBO, then exposes the color
  attachment as a cached `GpuTexture` the node emits on `out<GpuTexture>`.
  No `in<DrawFn>` — the draws that feed it are wired into its head event,
  exactly like the main `render_head`. (Mirrors how the main pass works;
  "only what reaches this head renders into this target.")
- **`texture_view` / `rd_renderer` = textured-quad nodes** emitting
  `Mesh` + `Surface` with the input `GpuTexture` as a sampler uniform —
  reuse the R0 texture-uniform binding (`Surface.images`/`GpuTexture`
  sampler path already in `render_region`). They become ordinary `draw`
  targets; no bespoke GL.
- **`glsl_effect` = a `Surface` carrying its `ShaderData` + the input
  `GpuTexture`**, drawn on a full-screen quad `Mesh` into either the main
  pass or another `render_target`. Chains compose:
  `draw → render_target → glsl_effect → texture_view`, all via Mesh/Surface
  + the seq event chain. Its GL program ownership moves into the boundary's
  program cache.

## render_region work
- FBO pass management: owned FBO + color/depth attachments per `target_id`,
  created/resized on demand, cached across frames; `begin_pass`/`end_pass`
  around a sub-queue; color attachment surfaced as a `GpuTexture` keyed by
  `target_id` (stable id so downstream sampler bindings are stable).
- Multiple ordered queues (main + N targets); the existing single
  `queue_` generalizes to a per-pass queue selected by which head a draw
  chained from. Replay order: targets first (dependencies), main pass last.
- Texture-uniform sampler binding already exists (R0); confirm it covers a
  render-target color attachment (GL texture id) as cleanly as a CPU-image
  upload.

## Files
- `components/render_region/render_region.hpp` / `.cpp` (FBO passes,
  per-pass queues, GpuTexture attachment surfacing)
- `components/render_region/render_region_nodes.hpp` (a `render_target`
  head-with-FBO node alongside `render_head`/`draw`)
- `components/render_nodes/*` (rewrite `RenderTargetNode` →
  render_target-pass node emitting `GpuTexture`; `TextureViewNode` →
  textured-quad Mesh+Surface)
- `components/rd_renderer/*` (→ textured-quad node; revive `rd_renderer.test`)
- `components/glsl_effect/*` (→ Surface+quad; program into boundary cache)
- `components/render_payloads/render_payloads.hpp` (if a render-target
  descriptor field is needed on `Surface`/`Mesh`)
- root `CMakeLists.txt` (re-enable the four subdirectories)
- `components/host_app/host_app.cpp`, `components/app/app.cpp` (re-register
  the revived nodes — BOTH shells)
- `assets/graphs/*` (an offscreen demo scene:
  `draw → render_target → glsl_effect → texture_view`)

## Acceptance
- A scene renders geometry into an offscreen target, runs a `glsl_effect`
  pass, and blits the result on-screen — verified by headless screenshot
  (`spectator` → POST → /screenshot), byte-comparable to the pre-park
  DrawFn rendering.
- `rd_gpu → rd_renderer` shows the reaction-diffusion field again.
- No node calls `glDraw*` / compiles a program / owns an FBO except
  `render_region` (and `rd_gpu`, the RD ping-pong resource-holder).
- Host + Quest green; `rd_renderer.test` back in the suite.

## Dependencies / notes
- Requires the Mesh/Surface render path + `render_region` texture binding
  (done) and DrawFn fully gone (done, ABI v8).
- Independent of the editor work. Can proceed anytime.
- `scene_snapshot`'s `SnapshotDrawFn` is the unrelated headless-capture
  typedef — not part of this.
