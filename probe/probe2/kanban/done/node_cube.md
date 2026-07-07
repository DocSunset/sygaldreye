# Node: `cube`

## Goal

Replace the hardcoded `state.scene_.cubes()` rendering loop in `app.cpp` (lines 429–433)
with a `cube` graph node. Each node renders one lit cube at a configurable position/scale
with configurable material, receiving light parameters from wired inputs (e.g. from
`sun_light` node).

After this node is implemented, `state.cube_mesh_` and `state.sun_light_` and the scene
cubes loop can be removed from `AppState` and `android_main`.

## Feasibility Assessment

**Mostly straightforward, one notable design constraint.**

The graph draw-call signature is `void(const Eigen::Matrix4f& pv)` where `pv = proj * view`.
The existing `CubeMesh::begin_batch` takes `view_pos` (world-space camera position) for
specular highlight calculation. This cannot be extracted from `pv` alone without knowing `P`
separately.

**Resolution:** each `CubeNode` holds its own `CubeMesh` instance and passes
`Eigen::Vector3f::Zero()` as an approximate view_pos. Specular will be slightly wrong
(computed as if camera is at the world origin) but diffuse is unaffected. This is an
accepted limitation; a future work item can pass view_pos via a dedicated port or change
the draw callback signature.

Alternatively, a simpler LitShader-less approach: use `flat_shader` with no specular. But
Blinn-Phong diffuse is worth keeping; just accept the specular approximation.

**Resource overhead:** each cube node owns a `CubeMesh` (one `LitShader` + one
`CubeGeometry`). A `LitShader` is a compiled GL program; creating many cube nodes is
costly. For the use cases anticipated (a handful of cubes), this is acceptable.

**Light inputs:** direction, color, intensity are wired float/vec3 inputs from `sun_light`
node. When not wired, they default to reasonable values via sliders.

## Component

`components/cube_node/`

```
cube_node/
  cube_node.hpp
  cube_node.cpp
  CMakeLists.txt
```

(No unit test: requires GL context. Acceptance tested via build + on-device.)

## Design

```cpp
class CubeNode {
public:
    static consteval std::string_view name() { return "cube"; }

    struct inputs {
        slider<"pos_x",       "", float, fp(-50.f), fp(50.f),  fp(0.f)>  pos_x;
        slider<"pos_y",       "", float, fp(-50.f), fp(50.f),  fp(0.f)>  pos_y;
        slider<"pos_z",       "", float, fp(-50.f), fp(50.f),  fp(-2.f)> pos_z;
        slider<"scale",       "", float, fp(0.01f), fp(20.f),  fp(0.5f)> scale;
        slider<"color_r",     "", float, fp(0.f),   fp(1.f),   fp(0.8f)> color_r;
        slider<"color_g",     "", float, fp(0.f),   fp(1.f),   fp(0.8f)> color_g;
        slider<"color_b",     "", float, fp(0.f),   fp(1.f),   fp(0.8f)> color_b;
        slider<"roughness",   "", float, fp(0.f),   fp(1.f),   fp(0.5f)> roughness;
        slider<"metalness",   "", float, fp(0.f),   fp(1.f),   fp(0.f)>  metalness;
        // Light inputs — wire from sun_light node
        port<"light_dir",     Eigen::Vector3f>                            light_dir;
        port<"light_color",   Eigen::Vector3f>                            light_color;
        port<"light_intensity", float>                                    light_intensity;
    } inputs;

    struct outputs {
        port<"render", DrawFn> render;
    } outputs;

    void operator()(double time_s);

private:
    CubeMesh mesh_;
    bool     initialized_ = false;

    void ensure_init();
};
```

`operator()` builds `model = translate(pos) * scale(s)`, sets up a `Light` from
the wired inputs (defaulting to the hardcoded values from app.cpp when light_intensity
is 0), sets `outputs.render.value` to a lambda capturing `this` + `model`.

The draw lambda:
```cpp
[this, model](const Eigen::Matrix4f& pv) {
    Light sun;
    sun.direction = light_dir_captured;   // captured from inputs at process() time
    sun.color     = light_color_captured;
    sun.intensity = light_intensity_captured;
    mesh_.begin_batch({&sun, 1}, Eigen::Vector3f::Zero());
    mesh_.draw(pv * model, model, mat_);
    CubeMesh::end_batch();
}
```

`ensure_init()` calls `mesh_.init()` once, guarded by `initialized_`. Since `init()`
requires a live GL context, it must be deferred to first `operator()` call (process is
called inside the frame loop when GL is active).

## App.cpp Cleanup

Remove from `AppState`:
- `CubeMesh cube_mesh_`
- `Light    sun_light_`

Remove from `android_main`:
- `state.cube_mesh_.init();`
- The sun_light computation block (lines ~378–390) — only if `node_sun_light` is done
- The `cube_mesh_.begin_batch / draw / end_batch` loop (lines ~429–433)

Update `kDefaultGraph` to include a `cube` node wired to `sun_light`.

## Registration

```cpp
#include "cube_node.hpp"
state.registry_.register_builtin(make_descriptor<CubeNode>());
```

## Acceptance Criteria

- `cube` node renders a lit cube at configurable position
- Light inputs wire from `sun_light` node
- `state.cube_mesh_` and `state.sun_light_` removed from AppState
- Scene cubes loop removed from render callback
- `sh/build.sh` passes
