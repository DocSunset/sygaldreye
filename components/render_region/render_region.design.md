# render_region

The render boundary — the GL "device" of rendering (the audio_region of
graphics). Draw nodes enqueue (Mesh, Surface) in tick order; render_region
owns ALL GPU resources and is the ONLY component that issues glDraw. GPU
caches are validated by payload versions, not wiped on graph swaps.

## Ports

- Inputs: `enqueue(Mesh, Surface)` (tick thread, GL-free); `begin_frame()`
  (clears the queue, advances the frame counter); `issue(view, proj, time_s)`
  (render thread, GL context current — replays the queue per eye);
  `notify_graph_swap()` (drops version-0 image-texture entries whose
  node-owned key address a new graph may reuse).
- Outputs: framebuffer draws; well-known uniforms injected unconditionally
  (`uMVP`, `uCameraRight/Up`, `uViewPos`, `uTime` — absent ⇒ location -1,
  silent no-op).
- Sources: draw/render_head/forest_draw nodes (the enqueuers); the shells'
  camera matrices; payload versions (`TriMeshData::version`,
  `ImageTex::version`, GLSL source content).
- Destinations: the current framebuffer; GL object lifetimes.
- Temporal couplings: begin_frame → tick (enqueues) → issue×eyes, same
  thread, same frame; a Span/ImageTex handed in must outlive the frame;
  producers must `touch()` mutated geometry before issue.
- Intended seams: `stats()`/`cached_*()` counters (tests observe cache
  behavior without GL introspection); `Mesh::dynamic` as a buffer-usage hint.

## Requirements

- Geometry cache keyed by `TriMeshData*`, validated by its globally unique
  version: mismatch ⇒ (re)upload (update_verts when topology is unchanged),
  match ⇒ bind only — the second eye never re-uploads.
- Program cache keyed by a hash of the GLSL source: N node instances of one
  shader compile once; survives graph swaps.
- Image textures keyed by `ImageTex::key`, re-uploaded on version/size change.
- Eviction: entries whose geometry died (weak_ptr expired) or unused for
  ~300 frames are deleted (GL objects freed) — no permanent accumulation.
- Per-instance Spans upload once per frame (frame-stamped slots per queue
  index); instanced N = min rows across attributes, one-time stderr warning
  on mismatch.
- Draw order = queue order = tick order; seq chains from render_head make it
  deterministic (see render_region_nodes.hpp).

## Allowed dependencies

`render_payloads`, `gl_program`, `gl_geometry`, `tri_mesh`, Eigen, GLES3.

## Owning package

`render`
