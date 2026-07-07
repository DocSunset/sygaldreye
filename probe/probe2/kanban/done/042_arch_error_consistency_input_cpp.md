# Make input error handling consistent and expose sync status to callers

**File:** `components/input/input.cpp`
**Principle:** Error paths must be handled consistently; silent failures and mixed logging conventions make callers unable to distinguish transient from fatal errors.

`Input::sync()` calls `xrSyncActions` and `xrLocateSpace` without checking or logging their return values (lines 78–102). On `xrLocateSpace` failure, `poses_[i].valid` is simply left false with no log entry — the caller cannot tell if the pose is unavailable due to normal tracking loss or a system error. This is inconsistent with the `xr_ok` helper used in `create()` (lines 8–12). `sync()` should use `xr_ok` on all XR calls and return a status value that distinguishes tracking loss from error.

## Acceptance
- `xrSyncActions` and `xrLocateSpace` results are checked via `xr_ok` or equivalent in `sync()`.
- `sync()` returns a value (enum or bool) indicating whether poses were successfully updated.
- All XR failures in `sync()` are logged with function name and result code.
- The build succeeds.
