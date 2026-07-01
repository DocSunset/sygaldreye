# Fix fragile vector resize pattern that silently discards controller poses

**File:** `components/scene/scene.cpp`
**Principle:** Data managed together should be structured so that invariants are maintained automatically; an API that requires callers to call two functions in the right order to avoid data loss is a design smell that should be resolved by making the correct state impossible to misuse.

`Scene::update()` calls `cubes_.resize(1)` to set the world cube, and `Scene::set_controller_poses()` also calls `cubes_.resize(1)` before appending controller cubes. If `update()` is called after `set_controller_poses()`, the controller cubes are silently truncated. The world cube and controller cubes are structurally different and should be stored separately (e.g., `CubeInstance world_cube_` and `std::array<std::optional<CubeInstance>, 2> controller_cubes_`), with `cubes()` assembling the span from both on demand or caching it after each mutation.

## Acceptance
- `Scene` stores the world cube and controller cubes in separate fields (not a single `cubes_` vector mixed together)
- Calling `update()` and `set_controller_poses()` in either order produces the correct combined cube list in `cubes()`
- No silent data loss when call order changes
- Build succeeds with no new warnings
