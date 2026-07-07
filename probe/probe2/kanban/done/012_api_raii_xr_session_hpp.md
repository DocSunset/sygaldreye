# Add destructor and move semantics to XrSessionObj

**File:** `components/xr_session/xr_session.hpp`
**Principle:** Every resource acquired must have a deterministic release path; owning types must implement RAII (destructor + move, deleted copy).

`XrSessionObj::create()` allocates an `XrSession` (via `xrCreateSession`) and an `XrSpace` (via `xrCreateReferenceSpace`), but the struct has no destructor, so both handles are leaked when the object goes out of scope. The struct is also copyable by default, allowing two instances to share and double-destroy the same handles. Add a destructor that calls `xrDestroySpace(worldSpace)` and `xrDestroySession(handle)` for non-null handles, delete copy operations, and add move operations that zero the moved-from handles.

## Acceptance
- `XrSessionObj` destructor destroys `worldSpace` and `handle` when non-null.
- Copy constructor and copy assignment are `= delete`.
- Move constructor and move assignment are defined and zero moved-from handles.
- Build succeeds with no new warnings.
