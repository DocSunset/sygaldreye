# Add destructor and move semantics to Input

**File:** `components/input/input.hpp`
**Principle:** Every resource acquired must have a deterministic release path; owning types must implement RAII (destructor + move, deleted copy).

`Input::create()` allocates an `XrActionSet`, an `XrAction`, and two `XrSpace` handles, none of which are ever freed. The struct has no destructor and is copyable by default. Add a destructor that calls `xrDestroySpace` for each `handSpaces_[i]`, `xrDestroyAction(poseAction_)`, and `xrDestroyActionSet(actionSet_)` for non-null handles. Delete copy operations and add move operations that zero the moved-from handles.

## Acceptance
- `Input` destructor destroys all three resource kinds (hand spaces, pose action, action set) when non-null.
- Copy constructor and copy assignment are `= delete`.
- Move constructor and move assignment are defined and zero moved-from handles.
- Build succeeds with no new warnings.
