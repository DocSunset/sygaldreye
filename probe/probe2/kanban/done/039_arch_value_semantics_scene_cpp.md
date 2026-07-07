# Replace set_controller_poses pointer parameters with value semantics

**File:** `components/scene/scene.cpp`
**Principle:** Prefer value types and std::optional over raw pointers and boolean validity flags to make invalid states unrepresentable and eliminate use-after-free risk.

`Scene::set_controller_poses` takes `const Eigen::Matrix4f*` pointers paired with `bool valid` flags (scene.hpp signature, scene.cpp lines 32–35). If the caller frees or invalidates the pointed-to matrices before `cubes()` is called, this is a use-after-free. The `valid` bool is redundant with a null pointer check, but the current API allows `valid=true` with a null pointer or `valid=false` with a non-null pointer. Using `std::optional<Eigen::Matrix4f>` removes the flag entirely and makes absence explicit.

## Acceptance
- `set_controller_poses` signature changes to `void set_controller_poses(std::optional<Eigen::Matrix4f> left, std::optional<Eigen::Matrix4f> right)` (or equivalent value-type form).
- Raw pointer parameters and paired `bool valid` flags are removed from both `scene.hpp` and `scene.cpp`.
- `app.cpp` call-site is updated to pass `std::optional` or equivalent.
- The build succeeds and hand cubes render correctly on-device.
