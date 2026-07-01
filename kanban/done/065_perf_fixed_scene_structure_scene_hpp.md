# Replace dynamic cubes_ vector with fixed-capacity array to prevent span invalidation

**File:** `components/scene/scene.hpp`
**Principle:** Prefer fixed-size scene representations over dynamic containers when the max count is known; avoid pointer/span invalidation hazards.

`Scene` holds `std::vector<CubeInstance> cubes_` (line 17) which is resized every frame by both `update()` and `set_controller_poses()`. The max cube count is statically bounded at 3 (1 world + 2 hand). Returning a `std::span` into a vector that is resized each frame creates a latent use-after-resize hazard: any code path that caches the span across a frame boundary would access freed memory. Additionally, `std::vector` carries heap-allocation overhead that is entirely unnecessary for a fixed-size collection.

Replace `std::vector<CubeInstance> cubes_` with `std::array<CubeInstance, 3> cubes_` and a `size_t cube_count_ = 0` field. Update `cubes()` to return `std::span<const CubeInstance>(cubes_.data(), cube_count_)`. Update `update()` and `set_controller_poses()` to write elements directly by index and set `cube_count_`.

## Acceptance
- `Scene` contains no `std::vector`; heap is not touched after construction.
- `cubes()` returns a valid span that is never invalidated by scene updates.
- All three cube slots (world + 2 hands) work correctly.
