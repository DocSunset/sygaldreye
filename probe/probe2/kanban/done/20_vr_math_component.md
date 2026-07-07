# 20 — vr_math component

New leaf component: turn OpenXR pose/fov into matrices. Pure math, no GL or XR state — one reason to change (matrix conventions).

## Component
- Name: `vr_math`, owning package: `render`.
- Allowed dependencies: Eigen, openxr headers (for `XrPosef`/`XrFovf` types only).

## Ports
- Input `projection(const XrFovf&, float near, float far) -> Eigen::Matrix4f` — asymmetric-frustum perspective from per-eye angles.
- Input `view(const XrPosef&) -> Eigen::Matrix4f` — inverse of the eye's world pose.
- Input `pose_to_world(const XrPosef&) -> Eigen::Matrix4f` — TR model matrix for placing content.

## Scope
- Header + cpp + gtest (`vr_math.test.cpp`) verifying: identity pose → identity-ish view; known fov → expected frustum terms; quaternion→rotation correctness.

## Depends on
- nothing (Eigen already a flake input)

## Acceptance
- `sh/test.sh` runs vr_math tests on-device, all pass.
