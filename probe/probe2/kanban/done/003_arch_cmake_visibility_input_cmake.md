# Fix cmake visibility: input should PRIVATE-link openxr_loader and log

**File:** `components/input/CMakeLists.txt`
**Principle:** CMake target dependencies must use PRIVATE unless the dependency's types appear in the component's public header; PUBLIC linkage leaks transitive dependencies to all consumers.

`input/CMakeLists.txt` links `openxr_loader` and `log` as PUBLIC. `input.hpp` does use `XrPosef`, `XrActionSet`, `XrAction`, and `XrSpace` from OpenXR headers, so `openxr_loader` could be argued as PUBLIC for header access. However, `log` is used only in `input.cpp` for error logging and is a pure implementation detail. Exposing `log` PUBLIC forces all consumers of `input` to link Android's log library even if they log through their own mechanism.

## Acceptance
- `log` is linked PRIVATE in `components/input/CMakeLists.txt`.
- The build still succeeds.
