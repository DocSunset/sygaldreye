# Preallocate cubes_ vector to avoid per-frame resize

**File:** `components/scene/scene.cpp`
**Principle:** Never allocate or resize containers on the per-frame hot path; preallocate everything at init time.

`Scene::set_controller_poses()` calls `cubes_.resize(1)` every frame (line 28), then conditionally `push_back`s up to 2 more elements. `Scene::update()` also calls `cubes_.resize(1)` every frame (line 16). The max scene size is 3 (1 world cube + 2 hand cubes), so there is no reason to repeatedly resize. These resize calls are potential allocation/deallocation events at 90 Hz, and they also invalidate any outstanding span into the vector.

Replace `cubes_.resize(1)` in `update()` with a direct assignment `cubes_[0] = ...`, and replace the `resize`/`push_back` pattern in `set_controller_poses()` with direct index writes. Reserve capacity 3 in the `Scene` constructor or via an `init()` method.

## Acceptance
- `cubes_` capacity is set to 3 exactly once (at construction or init), never changed thereafter.
- No call to `resize`, `push_back`, or any other mutating-capacity method appears on any per-frame code path.
- Existing behavior (1 world cube + up to 2 hand cubes) is preserved.
