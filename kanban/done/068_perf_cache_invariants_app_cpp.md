# Move hand model matrix computation out of render callback into scene update

**File:** `components/app/app.cpp`
**Principle:** Compute values that are invariant within a scope once and reuse; don't recompute per-eye or per-draw what is constant for the frame.

Inside the `render_frame` lambda (lines 732-743), `scale_m` and `local_T` are reconstructed as 4x4 identity matrices every frame even though they are constants that never change. More significantly, `pose_to_world(lh.pose) * local_T * scale_m` (two 4x4 multiplies = 128 FLOPs) is computed in the render callback rather than during scene update, and is wrapped in `.eval()` — which means Eigen computes it immediately but the result is discarded next frame rather than being cached. These are frame-invariant quantities that belong in `Scene::set_controller_poses()` or a scene update step, not the render path.

Compute `scale_m` and `local_T` once (as `constexpr` or `static const` matrices), and move hand model matrix computation (`pose_to_world * local_T * scale_m`) into `scene_.set_controller_poses()` or a new `scene_.update_hands()` method so the render callback is purely submission logic.

## Acceptance
- `scale_m` and `local_T` are not reconstructed inside the per-frame lambda.
- Hand model matrix computation (the `pose_to_world` multiply chain) does not appear inside `render_frame`'s callback.
- Hand cubes render correctly with the same visual placement.
