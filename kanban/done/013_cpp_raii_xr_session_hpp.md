# Add destructor to XrSessionObj to release XrSpace and XrSession handles

**File:** `components/xr_session/xr_session.hpp`
**Principle:** Every resource acquired must have a deterministic release path — either a destructor or a smart wrapper — so that resources are freed when the owning object goes out of scope.

`XrSessionObj` acquires `worldSpace` via `xrCreateReferenceSpace` and `handle` via `xrCreateSession`, but has no destructor, so both handles leak on destruction. A destructor must call `xrDestroySpace(worldSpace)` before `xrDestroySession(handle)`, each guarded by a `XR_NULL_HANDLE` check. The copy constructor and copy-assignment should be deleted to prevent aliased ownership.

## Acceptance
- `XrSessionObj` has a destructor that calls `xrDestroySpace` then `xrDestroySession` with null-handle guards
- Copy constructor and copy-assignment operator are deleted
- Build succeeds with no new warnings
