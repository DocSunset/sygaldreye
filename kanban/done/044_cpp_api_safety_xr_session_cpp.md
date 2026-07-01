# Replace C-style cast with reinterpret_cast when loading xrGetOpenGLESGraphicsRequirementsKHR

**File:** `components/xr_session/xr_session.cpp`
**Principle:** OpenGL/OpenXR API calls must use explicit C++ casts (`reinterpret_cast`/`static_cast`) rather than C-style casts, making the intent and danger of each cast visible to reviewers and static analysis tools.

`XrSessionObj::create()` contains `(PFN_xrVoidFunction*)&getReqs` (and the same pattern appears in `app/xr.cpp`), which is a C-style cast that silently reinterprets the function-pointer address. This should be `reinterpret_cast<PFN_xrVoidFunction*>(&getReqs)` to make the dangerous type pun explicit and searchable. The same fix applies to the `initLoader` cast in `xr.cpp`.

## Acceptance
- All `(PFN_xrVoidFunction*)` C-style casts in `xr_session.cpp` are replaced with `reinterpret_cast<PFN_xrVoidFunction*>(...)`
- The equivalent casts in `app/xr.cpp` are updated in the same commit
- Build succeeds with no new warnings; `-Wold-style-cast` does not flag the changed lines
