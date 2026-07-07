# Return a result from Input::sync instead of silently continuing

**File:** `components/input/input.cpp`
**Principle:** Returning typed error codes or results rather than logging and continuing allows tests to verify that the right error was emitted and to distinguish error categories.

`Input::sync()` returns `void`. If `xrSyncActions` or either `xrLocateSpace` call fails, the function logs nothing for `xrSyncActions` failure and silently marks poses invalid for locate failures — there is no way for callers or tests to distinguish "pose tracking momentarily lost" from "action sync failed entirely" from "space location returned an XR error." Changing the signature to `bool sync(...)` (or a richer enum) and returning `false` on `xrSyncActions` failure makes the distinction testable and lets `android_main` react appropriately.

## Acceptance
- `Input::sync()` returns `bool` (or a richer result type); `true` means action sync succeeded, `false` means it did not
- `xrSyncActions` failure causes `sync` to return `false` immediately
- The caller in `app.cpp` checks the return value
- `input.hpp` is updated to match the new signature
