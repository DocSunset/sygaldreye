# 13 — xrCreateSession

Create the GLES-bound session.

## Component
`xr` / `xr_session`

## Scope
- `xrCreateSession` with instance + system (sources from `app`) and the
  graphics binding (source from item 12, injected — no dep on `render`).
- Start `xr_session.design.md` (sources, temporal couplings, layer seam).

## Depends on
- 10, 12

## Acceptance
- Session created without error; handle logged.
