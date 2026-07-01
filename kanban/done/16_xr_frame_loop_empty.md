# 16 — empty frame loop

Run the wait/begin/end frame cycle.

## Component
`xr` / `xr_session`

## Scope
- Per frame: `xrWaitFrame` → `xrBeginFrame` → `xrEndFrame` (zero layers).
- Respect `shouldRender`; honor predicted display time.
- Expose a layer-submission seam for `render` to fill.

## Depends on
- 14, 15

## Acceptance
- **Milestone:** session FOCUSED, frame loop runs steadily, clean logcat.
