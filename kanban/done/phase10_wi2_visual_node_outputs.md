# WI-2: Visual node outputs (draw_call ports)

## Summary
All visual nodes need `struct outputs { port<"render", DrawFn> render; } outputs;`
and must set `outputs.render.value` in their `operator()(double)`.

## Nodes to update
- `sky_dome` (SkyDome) — has `draw(Matrix4f)`, no operator()
- `water_surface` (WaterSurface) — has `draw(mvp, model, view_pos)`, no operator()
- `lissajous` (Lissajous) — has `operator()(double)` and `draw(Matrix4f)`
- `aurora` (Aurora) — has `operator()(double)` and `draw(Matrix4f)`
- `chladni` (Chladni) — has `operator()(double)` and `draw(Matrix4f)`
- `terrain_generator` (TerrainRenderer) — has `operator()(double)` and `draw(mvp, model, view_pos)`
- `particle_system` (ParticleSystem) — has `operator()(double)` and `draw(vp, right, up)`
- `reaction_diffusion` (ReactionDiffusion) — has `operator()(double)` and `draw(Matrix4f)`
- `rd_gpu` (RdGpu) — already has `struct outputs`; add `render` port alongside `concentration`

## Notes
- For nodes with non-trivial draw signatures (water, terrain, particle), wrap with a
  lambda that bakes in the extra params (model matrix, view pos, camera vectors).
- `sky_dome` and `water_surface` have no `operator()` — they must gain one.
- `rd_gpu` already has outputs struct with `concentration` texture port.

## Acceptance
- All listed nodes have `struct outputs { port<"render", DrawFn> render; } outputs;`
- `operator()(double)` sets `outputs.render.value`
- `sh/build.sh` passes
- Commit
