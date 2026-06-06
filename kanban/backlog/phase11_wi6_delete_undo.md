# WI-6: Delete and undo

Point at a node card or wire for 1 second to delete it. Thumbstick-left gesture undoes last change.

## Deliverable

Dwell timer in `VrEditor`; `delta_time_s` and `thumbstick_left` parameters added to `update()`; single-level undo buffer.

## Acceptance

- 1 second dwell on card/wire deletes it and emits `GraphEdit`
- Thumbstick-left restores previous graph
- `sh/build.sh` passes
