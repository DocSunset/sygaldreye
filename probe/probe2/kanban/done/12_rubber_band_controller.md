# rubber_band_controller component

A composited VR widget: a grabbable anchor sphere connected by a rubber band to a grabbable control sphere. The basic version is unconstrained — the control sphere can be placed freely in 3D space. Specialized versions (distance-clamped sliders, axis-projected sliders, etc.) are built on top by wrapping or extending this component.

## Behaviour

- **Anchor sphere**: fixed in world space unless grabbed.
- **Control sphere**: opaque, small. Unconstrained in the basic version; follows the grabbing hand freely.
- **Rubber band**: thin cylinder drawn between the two sphere surfaces.
- **Label**: string rendered below the anchor, produced by an injected `label_fn`. Default shows the offset magnitude.
- **Grab anchor**: moves the entire widget (anchor + control maintain relative offset).
- **Grab control**: repositions the control sphere; anchor stays fixed.
- **Offset output**: `Eigen::Vector3f offset() const` returns the vector from anchor to control sphere. Callers derive whatever scalar they need (distance, projection onto an axis, angle, etc.).

## Approach

- `rubber_band_controller.hpp` declares `RubberBandController` with:
  - `create(Eigen::Vector3f anchor_pos, std::function<std::string(Eigen::Vector3f)> label_fn = default_label)`
  - `update(std::span<HandState const> hands)` — runs grab detection, moves control sphere
  - `draw(Eigen::Matrix4f const& view_proj)` — emits all draw calls
  - `Eigen::Vector3f offset() const`
- `label_fn` is the **vector-to-label seam**: injected at construction, called each frame with the current offset to produce the label string. The default implementation returns the magnitude formatted as a decimal.
- `rubber_band_controller.cpp` owns two `GrabTarget`s, a `SphereMesh`, a `CylinderMesh`, an `RgbaShader`, and a `TextMesh` instance.
- No clamping or projection in this component; those belong in specialized wrappers that consume `offset()` and/or supply a custom `label_fn`.

## Note: grab arbitration

This work item has the controller running `grab_detector` internally over its own two `GrabTarget`s. That works for one widget, but if a second grabbable thing is ever in the scene, the same hand could grab both simultaneously — each widget's detector runs independently.

The fix when that day comes: a `GrabRouter` that owns all `GrabTarget`s across widgets and runs `grab_detector` once per frame. Widgets register targets on creation and unregister on destruction; `GrabRouter` routes the resulting grab state back to each target.

## Acceptance

- Moving the control sphere freely in 3D updates `offset()` to the correct vector
- Grabbing the anchor moves the whole widget; control sphere maintains its relative position
- Label updates each frame via `label_fn`; a custom `label_fn` is demonstrably injectable
- On release the widget holds its last position
- No GL errors during normal use
- `rubber_band_controller.design.md` present; each `.cpp` under 100 lines of substantive code

## Dependencies

- Items 01–06
- `text_rendering_msdf` (TextMesh)
