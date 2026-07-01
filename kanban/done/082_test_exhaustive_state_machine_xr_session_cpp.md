# Add state machine tests for XrSessionObj::poll_events

**File:** `components/xr_session/xr_session.cpp`
**Principle:** Every state machine transition should be covered by at least one test; unexercised paths are invisible bug incubators.

`poll_events()` handles three distinct transitions: READY → `xrBeginSession` + `sessionRunning_=true`; STOPPING → `xrEndSession` + `sessionRunning_=false`; EXITING/LOSS_PENDING → `quit_=true`. None of these transitions has any test. Bugs such as calling begin twice, ending before ready, or setting `sessionRunning_` incorrectly on failure would go undetected. The state and flags are public members of `XrSessionObj`, so a test fixture can inject a fake `XrEventDataSessionStateChanged` by directly setting `state` and calling the transition logic — or, better, by factoring the transition logic into a pure function `handle_session_state_change(XrSessionState new_state, ...) -> SessionEffect` that can be tested without a live XR runtime.

## Acceptance
- Tests exist for: READY transition sets `sessionRunning_=true`; STOPPING transition sets `sessionRunning_=false`; EXITING sets `quit_=true`; LOSS_PENDING sets `quit_=true`
- Tests can run offline (no XR runtime required), either via a pure transition function or via a stub/fake XR handle
- All four transitions above are covered
