# WI-1: VisualNode<Derived> CRTP Base

## Goal

Extract the identical `operator()(double)` lifecycle from all 8 visual nodes into a
single CRTP base class `VisualNode<Derived>` that provides the pattern once:

```cpp
void operator()(double time_s) {
    auto& d = static_cast<Derived&>(*this);
    d.sync_params();          // copy slider values → internal params
    d.update(float(time_s));  // advance simulation
    outputs.render.value = [&d](const Eigen::Matrix4f& vp){ d.draw(vp); };
}
```

Each node must implement `sync_params()`, `update(float)`, and `draw(const Eigen::Matrix4f&)`.
The base provides the `outputs.render` port.

## Affected Nodes

aurora, lissajous, chladni, reaction_diffusion, water_surface, sky_dome,
terrain_generator, particle_system

## Acceptance Criteria

- New component `components/visual_node/visual_node.hpp` with the CRTP base
- All 8 visual nodes refactored to inherit from `VisualNode<T>`
- Each node's `operator()` body is replaced by the base (no duplicated body remains)
- `sh/build.sh` passes (host + Android targets)
- All existing `integration_real_nodes` tests still pass
- Snapshot binaries still build and render correctly

## Notes

`sky_dome` is slightly non-standard: it outputs both `render` and scalar ports
(`sun_dir`, `sun_color`, etc.) from `operator()`. The base should allow derived
classes to additionally populate their own outputs after calling the base, or the
`sync_params()` step should cover it.
