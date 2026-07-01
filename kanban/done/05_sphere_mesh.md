# sphere_mesh component

Upload `SphereGeometry` to the GPU and expose a VAO-based draw call. Parallel to `cube_mesh`.

## Approach

- `sphere_mesh.hpp` declares `SphereMesh` with a `create(SphereGeometry const&)` factory and a `draw()` method.
- `sphere_mesh.cpp` manages one VAO, one VBO (interleaved position+normal), and one EBO. `draw()` issues `glDrawElements`.
- Destructor releases GL objects. Move constructor/assignment swap them.

## Acceptance

- `draw()` issues no GL errors (checked with `glGetError`)
- Destructor and move semantics do not leak or double-free GL objects
- VAO attribute layout matches `lit_shader`: position at location=0, normal at location=1
- `sphere_mesh.design.md` present; each `.cpp` under 100 lines of substantive code

## Dependencies

- `sphere_geometry` (item 01)
- `lit_shader` (item 04)
