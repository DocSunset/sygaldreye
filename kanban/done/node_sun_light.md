# Node: `sun_light`

## Goal

Replace the hardcoded day-cycle sun calculation in `app.cpp` (lines 378–390) with a
`sun_light` graph node that emits direction, color, and intensity outputs each frame.
After this node exists, the hardcoded block and `state.sun_light_` member can be
removed from `AppState`; cube nodes will wire to the sun_light outputs instead.

## Feasibility Assessment

**Straightforward.** The existing logic is pure math with no GL, no XR, no threading.
It maps cleanly to a stateless node: inputs are sliders (day period, phase offset),
process() runs the same trig, outputs are floats/vec3.

The only design question is whether to output the light as separate scalar ports or as
a `vec3` direction + `vec3` color + `float` intensity. Separate scalars are simpler
(no new port-schema complications); vec3 outputs are more composable. Use vec3 +
float — they are already in `PortValue` and `push_outputs` handles them.

`sun_elevation_out` (float) should also be emitted so the node can replace `sky_dome`
as the elevation source for other nodes (water_surface etc.), and/or the sky_dome's
own `sun_elevation_out` can be wired into a `sun_light` node's `elevation_in` as an
alternative driver. For the first implementation, make it self-contained with sliders.

## Component

`components/sun_light/`

```
sun_light/
  sun_light.hpp
  sun_light.cpp
  sun_light.test.cpp
  CMakeLists.txt
```

## Design

```cpp
class SunLight {
public:
    static consteval std::string_view name() { return "sun_light"; }

    struct inputs {
        slider<"day_period_s",  "", float, fp(10.f),  fp(3600.f), fp(180.f)> day_period_s;
        slider<"phase_offset",  "", float, fp(0.f),   fp(1.f),    fp(0.25f)> phase_offset;
        slider<"min_intensity", "", float, fp(0.f),   fp(1.f),    fp(0.05f)> min_intensity;
        slider<"max_intensity", "", float, fp(0.f),   fp(5.f),    fp(1.3f)>  max_intensity;
    } inputs;

    struct outputs {
        port<"direction",       Eigen::Vector3f> direction;
        port<"color",           Eigen::Vector3f> color;
        port<"intensity",       float>           intensity;
        port<"elevation_norm",  float>           elevation_norm; // -1..1
    } outputs;

    void operator()(double time_s);
};
```

`operator()` replicates the logic from `app.cpp` lines 378–390, reading sliders for
the day period and phase offset. No GL, no XR.

## App.cpp cleanup

After registering this node, the block in `android_main` that computes `state.sun_light_`
(approximately lines 378–390) must be removed, and `state.sun_light_` removed from
`AppState`. Cube nodes wired to `sun_light.direction` / `.color` / `.intensity` replace
the current `begin_batch({&state.sun_light_,1}, view_pos)` call.

The `state.cube_mesh_` / `scene_.cubes()` loop (lines 429–433) can be removed once cube
nodes exist; that cleanup belongs to `node_cube.md`. For now, only remove the standalone
sun_light calculation; leave the cube_mesh loop in place (it will have a dummy light until
cube nodes wire it).

Actually: since cube_mesh still references `state.sun_light_`, keep sun_light_ in AppState
until cube nodes are done. Defer the app.cpp removal to `node_cube.md` which includes both.

## Registration

`state.registry_.register_builtin(make_descriptor<SunLight>());`
Add `#include "sun_light.hpp"` to app.cpp.

## Test

`sun_light.test.cpp` — unit tests (no GL needed):
- At phase 0.25 (midday), elevation_norm ≈ 1.0, intensity ≈ max_intensity, direction.y ≈ -1
- At phase 0.0 (midnight), elevation_norm ≈ -1.0, intensity ≈ min_intensity
- Color warm component ≈ 1.0 at midday, lower at horizon

## Acceptance Criteria

- Component builds with `sh/build.sh`
- Unit tests pass
- `sun_light` registered in app.cpp
- Default graph updated to include a `sun_light` node
- No regressions in existing nodes
