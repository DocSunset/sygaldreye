# Make XrSessionObj handle fields private

**File:** `components/xr_session/xr_session.hpp`
**Principle:** Implementation details must be private; only the minimal necessary interface is public, so callers cannot depend on internal layout or accidentally mutate internal state.

`XrSessionObj` exposes `instance`, `handle`, `worldSpace`, `state`, `quit_`, and `sessionRunning_` as public data members, allowing any caller to read or modify them directly. The existing accessor methods (`get()`, `worldSpace_()`, `should_quit()`, `session_running()`, `should_render()`) already provide the needed read access. Move all data members to `private`; the existing accessors cover all external read needs. The naming inconsistency (`quit_` and `sessionRunning_` use private-style suffixes but are public) is a symptom of this problem.

## Acceptance
- All data members of `XrSessionObj` are `private`.
- The existing public accessor methods are unchanged.
- No external code directly reads or writes `instance`, `handle`, `worldSpace`, `state`, `quit_`, or `sessionRunning_`.
- Build succeeds with no new warnings.
