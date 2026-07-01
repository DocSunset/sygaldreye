# 19 — projection layer submit

Submit the rendered eyes as a projection layer.

## Component
`render` / `renderer`

## Scope
- `xrLocateViews` (reference space, predicted time).
- Build per-view `XrCompositionLayerProjectionView` (pose/fov/sub-image) +
  `XrCompositionLayerProjection`; pass to the `xr_session` layer seam (item 16).

## Depends on
- 18

## Acceptance
- **Milestone:** both eyes show a slowly color-cycling field, correctly
  head-locked, stable frame rate.
