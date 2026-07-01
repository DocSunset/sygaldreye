# Replace static bool logged in Input::sync with instance state

**File:** `components/input/input.cpp`
**Principle:** Mutable static variables introduce hidden global state and thread-safety hazards; replace with instance state or thread-safe one-time primitives.

`Input::sync` uses `static bool logged = false` to log the first valid hand pose once. This is a function-local static that persists for the entire process lifetime, is not thread-safe, and makes the `Input` class non-reentrant. If two `Input` instances are ever created (or if `sync` is called from two threads), the flag is shared incorrectly. Move `logged` to a `bool` member of `Input` (e.g., `bool pose_logged_ = false`) so each instance tracks its own logging state independently.

## Acceptance
- The `static bool logged` variable is removed from `Input::sync`.
- A `bool pose_logged_ = false` member is added to the `Input` struct.
- The logging logic references `pose_logged_` (the instance member) instead.
- Build succeeds with no new warnings.
