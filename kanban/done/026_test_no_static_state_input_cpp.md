# Move static logged flag in Input::sync to an instance member

**File:** `components/input/input.cpp`
**Principle:** Static local variables for mutable state persist across test runs, making test isolation impossible without a process restart; use instance members instead.

`Input::sync()` contains `static bool logged` which gates a one-shot diagnostic log of the first successfully located hand pose. Because it is a static local, it persists for the lifetime of the process: a second `Input` object, or a test that constructs one `Input` per test case, will silently skip the first-pose log on all but the very first test run. Moving `logged` to an `Input` member field (initialized `false`) restores correct per-object semantics and allows tests to verify logging behavior by inspecting a fresh instance.

## Acceptance
- `logged` is an `Input` member field initialized to `false`, not a `static` local in `sync()`
- `sync()` reads and writes `this->logged`
- No `static` keyword remains on mutable locals in `sync()`
