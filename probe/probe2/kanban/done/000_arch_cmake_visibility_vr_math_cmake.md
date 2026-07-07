# Fix cmake visibility: vr_math should PRIVATE-link openxr_loader

**File:** `components/vr_math/CMakeLists.txt`
**Principle:** CMake target dependencies must use PRIVATE unless the dependency's types appear in the component's public header; PUBLIC linkage leaks transitive dependencies to all consumers.

`vr_math/CMakeLists.txt` links `openxr_loader` as PUBLIC. While `vr_math.hpp` uses `XrFovf`, `XrPosef` etc., these types come from `openxr/openxr.h` which is included via the `XR_USE_*` define chain — not from the loader library itself. The loader provides runtime symbols, not headers. Exposing it PUBLIC causes all consumers of `vr_math` to link the loader even if they don't call XR functions directly.

## Acceptance
- `openxr_loader` is linked PRIVATE in `components/vr_math/CMakeLists.txt`.
- Header includes for OpenXR types in `vr_math.hpp` continue to work (they are pulled in via include path, not the loader link).
- The build still succeeds and `vr_math_test` passes on-device.
