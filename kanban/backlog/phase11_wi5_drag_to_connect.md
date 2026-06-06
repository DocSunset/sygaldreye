# WI-5: Drag-to-connect

Grip an output port handle, drag to an input port handle of matching kind, release to create an edge.

## Deliverable

Drag state machine in `VrEditor`; `grip_right` parameter added to `update()`; ghost wire rendered while dragging.

## Acceptance

- Grip on output handle starts drag
- Release over compatible input handle emits `GraphEdit` with new edge
- Ghost wire rendered during drag
- `sh/build.sh` passes
