# Add destructor to Input to release XrActionSet and XrAction handles

**File:** `components/input/input.hpp`
**Principle:** Every resource acquired must have a deterministic release path — either a destructor or a smart wrapper — so that resources are freed when the owning object goes out of scope.

`Input` acquires `actionSet_` via `xrCreateActionSet`, `poseAction_` via `xrCreateAction`, and two `handSpaces_` via `xrCreateActionSpace`, but has no destructor. All three handle types leak when the `Input` object is destroyed. A destructor must call `xrDestroySpace` for each hand space, `xrDestroyAction` for `poseAction_`, and `xrDestroyActionSet` for `actionSet_`, in that order (children before parents). The copy constructor and copy-assignment operator should also be deleted.

## Acceptance
- `Input` has a destructor that destroys hand spaces, pose action, and action set in reverse-creation order using `XR_NULL_HANDLE` guards
- Copy constructor and copy-assignment operator are deleted
- Build succeeds with no new warnings
