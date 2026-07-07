# 24 — scene component

New package `scene` (per architecture). Owns app content: what cubes exist and where. No GL or XR internals — communicates via plain pose/transform data so it has its own reasons to change (content/logic only).

## Component
- Name: `scene`, owning package: `scene`.
- Allowed dependencies: Eigen only.

## Ports
- Input `update(double time)` — advance animation (spin the floating cube).
- Output `cubes() -> std::span<const CubeInstance>` where `CubeInstance { Eigen::Matrix4f model; }` (or pos+rot+scale).
- Seam: controller-attached cubes (item 28) appended each frame via an input port (`set_controller_poses(...)`).

## Scope
- Header + cpp + gtest: after `update(t)` the floating cube's model matrix rotates monotonically with `t`; cube sits at a fixed world position (e.g. 1m forward at eye height).

## Depends on
- nothing above scene

## Acceptance
- `sh/test.sh` scene tests pass on device.
