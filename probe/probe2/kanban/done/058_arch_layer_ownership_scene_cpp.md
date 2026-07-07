# Fix scene resize pattern: separate world cube and hand cube ownership

**File:** `components/scene/scene.cpp`
**Principle:** Each layer should own its domain's logic; shared mutable state with implicit ordering between callers creates hidden temporal coupling.

`Scene::update()` calls `cubes_.resize(1)` to reset to just the world cube (line 17), and `set_controller_poses()` also calls `cubes_.resize(1)` before appending hand cubes (line 32). `app.cpp` must call `update()` before `set_controller_poses()` — if the order is reversed, hand cubes are silently erased. This ordering constraint is undocumented and unenforced. Separate data members (`world_cube_` and `hand_cubes_[2]`) would eliminate the dependency entirely.

## Acceptance
- `Scene` stores the world cube and hand cubes in separate data members (e.g. `CubeInstance world_cube_` and `std::array<std::optional<CubeInstance>, 2> hand_cubes_`).
- `cubes()` assembles the final span from both members on-the-fly or into a local buffer.
- `update()` and `set_controller_poses()` (or its successor) no longer share a `resize` dependency.
- The build succeeds and cubes render correctly on-device.
