# Store and delete the EBO in CubeMesh to fix GPU memory leak

**File:** `components/cube_mesh/cube_mesh.cpp`
**Principle:** Every resource acquired must have a deterministic release path — either a destructor or a smart wrapper — so that resources are freed when the owning object goes out of scope.

`CubeMesh::init()` creates an EBO via `glGenBuffers` (line ~73) but never stores the handle and never calls `glDeleteBuffers` on it. The handle is lost after `init()` returns, leaking GPU buffer memory for the lifetime of the process. The fix is to add `ebo_` as a member of `CubeMesh` (alongside the existing `vao_` and `vbo_`), store the generated handle there, and delete it in `CubeMesh`'s destructor alongside `vbo_` and `vao_`.

## Acceptance
- `CubeMesh` has an `ebo_` member (type `GLuint`, default 0)
- `init()` stores the generated EBO handle in `ebo_`
- A `~CubeMesh()` destructor calls `glDeleteBuffers`/`glDeleteVertexArrays` on all three handles when non-zero
- Build succeeds; no GL resource warnings
