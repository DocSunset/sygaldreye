# 15 — session state machine

Drive session begin/end from events.

## Component
`xr` / `xr_session`

## Scope
- `xrPollEvent` loop; on `SessionStateChanged`: `xrBeginSession` at READY,
  `xrEndSession` at STOPPING, exit at EXITING/LOSS_PENDING.
- Expose current state / "should-render" as output ports.

## Depends on
- 13

## Acceptance
- logcat shows transition to READY → SYNCHRONIZED → VISIBLE → FOCUSED.
