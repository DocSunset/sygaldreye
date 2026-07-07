# Cache proj*view product per eye; hoist projection outside cube loop

**File:** `components/renderer/renderer.cpp`
**Principle:** Compute values that are invariant within a scope once and reuse; don't recompute per-eye or per-draw what is constant for the frame.

In `render_eyes()` (lines 207-212), `projection()` and `view()` are called inside the per-eye loop, and then `proj * v * cube.model` is computed fresh for every cube within that eye. `projection()` calls `std::tan()` four times and builds a 4x4 matrix; `view()` builds another 4x4 matrix. Both are constant for all cubes within one eye. The `proj * v` product (64 FLOPs) is also constant per eye and is recomputed for every cube.

Compute `proj` and `v` once before the cube loop (they already are, but the `proj * v` product is not cached). Add `Eigen::Matrix4f pv = proj * v;` before the cube loop and use `pv * cube.model` inside it, eliminating one 4x4 multiply per cube per eye.

## Acceptance
- `projection()` and `view()` are called exactly once per eye per frame.
- `proj * v` is computed once per eye and stored as `pv`; the cube loop uses `pv * cube.model`.
- Rendered output is visually identical.
