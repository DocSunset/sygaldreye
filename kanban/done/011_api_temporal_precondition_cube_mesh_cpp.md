# Assert init() precondition in CubeMesh::draw

**File:** `components/cube_mesh/cube_mesh.cpp`
**Principle:** Initialize-before-use preconditions must be enforced by assertions or types, not only by caller convention.

`CubeMesh::draw` calls `prog_.use()`, `prog_.uniform()`, and `glBindVertexArray(vao_)` without checking whether `init()` was called. If `draw` is invoked on a default-constructed `CubeMesh` (where `vao_ == 0`), `glBindVertexArray(0)` unbinds any current VAO and `glDrawElements` draws nothing (or garbage), with no error. Add `assert(vao_ != 0)` at the top of `draw()` to make the precondition explicit and catch uninitialized use immediately in debug builds.

## Acceptance
- `CubeMesh::draw` begins with `assert(vao_ != 0)`.
- The assertion fires in debug builds if `draw` is called before `init`.
- `<cassert>` is included in `cube_mesh.cpp`.
- Build succeeds with no new warnings.
