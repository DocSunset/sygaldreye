# Render text on right hand cube faces

Using `TextMesh` from `text_rendering_msdf`, render labelled text on four faces of the right hand cube:

- **Top**: "top"
- **Bottom**: "bottom"
- **Forward** (face visible to someone standing in front of me as I point the controller at them): "forward"
- **Toward user** (face I see in the same situation): "toward user"

## Dependencies

- `text_rendering_msdf` must be complete (`TextMesh` available)

## Architecture

Mirrors the existing cube pattern: `scene` produces data, `render` draws it.

- **`scene` exposes `labels()`** — a list of `{string, world_transform}` pairs, parallel to `cubes()`. Face anchor math (mapping text onto a cube face) lives in `scene`; it produces world-space transforms. `scene` stays GL-free.
- **`app.cpp` dispatches** — iterates `scene_.labels()` and calls `text_mesh_.draw(label.text, label.transform)`, exactly as it does for cubes.
- **`TextMesh` draws** — no knowledge of cube faces; just draws `{string, transform}` pairs.

Requires settling which local axis is the controller's pointing direction before implementing face anchors (determines "forward" vs "toward user").

## Acceptance

- The right hand cube displays the correct label on each of the four named faces
- Text is legible and correctly oriented when each face is viewed head-on
- Text remains sharp at close viewing distance (inherits from MSDF)
- `scene` has no GL dependency; face anchor logic lives entirely in `scene`
- `labels()` is a reusable port — not hardcoded to cube faces
