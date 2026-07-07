# WI-4: XR source node outputs

## Summary
`HeadPoseNode`, `LeftControllerNode`, `RightControllerNode` currently store pose in
private members. They need `struct outputs` with scalar ports so the pose data can flow
along graph edges.

## Work

### HeadPoseNode
Add `struct outputs` with 7 float ports (pos_x/y/z, rot_x/y/z/w).
Update `set_pose()` to write output ports in addition to `pose_`.

### LeftControllerNode / RightControllerNode
Same 7 pose ports, plus:
- `port<"trigger", float> trigger;`
- `port<"grip", float> grip;`
- `port<"thumbstick_x", float> thumbstick_x;`
- `port<"thumbstick_y", float> thumbstick_y;`

Update `set_state()` to write all these. Current signature is
`set_state(const XrPosef& p, bool trigger)` — add grip and thumbstick params, or use
separate setters if cleaner.

Use `slider<>` with fp() wrappers rather than raw `port<>` if range metadata is desired
for the VR editor; otherwise `port<Name, float>` is fine (scalar port).

## Acceptance
- All 3 nodes have typed output ports
- `set_pose` / `set_state` writes them
- `sh/build.sh` passes
- Commit
