# Add destructor and move semantics to CubeMesh

**File:** `components/cube_mesh/cube_mesh.hpp`
**Principle:** Every resource acquired must have a deterministic release path; owning types must implement RAII (destructor + move, deleted copy).

`CubeMesh::init()` calls `glGenVertexArrays` and `glGenBuffers`, storing handles in `vao_` and `vbo_`, but there is no destructor to call `glDeleteVertexArrays` and `glDeleteBuffers`. The EBO created in `init()` is also immediately orphaned (its local `ebo` variable goes out of scope with no delete, and it is not stored anywhere — this is a GL resource leak). Add a destructor that frees `vao_` and `vbo_`, store the EBO as a member and free it in the destructor, delete copy operations, and add move operations.

## Acceptance
- `CubeMesh` has a destructor calling `glDeleteVertexArrays(1, &vao_)` and `glDeleteBuffers` for all GL buffers when non-zero.
- The EBO is stored as a member and freed in the destructor.
- Copy constructor and copy assignment are `= delete`; move operations are defined.
- Build succeeds with no new warnings.
