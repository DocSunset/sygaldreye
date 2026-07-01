# Replace magic stride and offset literals in glVertexAttribPointer calls

**File:** `components/cube_mesh/cube_mesh.cpp`
**Principle:** Magic numbers embedded in expressions must be replaced with named `constexpr` constants so that the intent is self-documenting and adjustments require changing only one definition.

`CubeMesh::init()` calls `glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, (void*)0)` and `glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, (void*)12)`. The values `24` (stride: 6 floats × 4 bytes) and `12` (color offset: 3 floats × 4 bytes) are unexplained. Additionally `VERTS` and `IDX` are declared `static const` where `static constexpr` better expresses compile-time immutability. Define `constexpr GLsizei kVertexStride = 6 * sizeof(float)` and `constexpr size_t kColorOffset = 3 * sizeof(float)` and use them; also change `static const` to `static constexpr` for the two data arrays.

## Acceptance
- `kVertexStride` and `kColorOffset` are defined as `constexpr` values and used in both `glVertexAttribPointer` calls
- `VERTS` and `IDX` are declared `static constexpr`
- Build succeeds with no new warnings
