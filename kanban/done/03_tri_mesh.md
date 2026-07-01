# tri_mesh component

A general-purpose GPU mesh for arbitrary triangle geometry with per-vertex position, normal, and RGBA color. The substrate for terrain, trees, rocks, water, and any geometry that doesn't fit the sphere/cube/cylinder mold.

## Approach

- `tri_mesh.hpp` declares:
  ```cpp
  struct TriVertex {
      Eigen::Vector3f position;
      Eigen::Vector3f normal;
      Eigen::Vector4f color; // RGBA, linear
  };

  struct TriMeshData {
      std::vector<TriVertex> vertices;
      std::vector<uint32_t>  indices;
  };

  class TriMesh {
  public:
      static TriMesh create(TriMeshData const&);
      void update(TriMeshData const&); // re-upload for animated meshes
      void draw() const;
      // RAII: destructor releases GL objects; move-only
  };
  ```
- `tri_mesh.cpp` manages one VAO, one VBO (interleaved), one EBO. `update()` uses `glBufferSubData` when the vertex count is unchanged, `glBufferData` otherwise.
- `tri_mesh.test.cpp`: verify no GL errors from `create` and `draw`; verify `update` with same and different sizes.

## Acceptance

- A hand-constructed `TriMeshData` (e.g. one triangle) renders without GL errors (on-device)
- `update()` with resized data does not corrupt rendering
- `tri_mesh.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `flat_shader` (item 02) — intended draw-time shader
