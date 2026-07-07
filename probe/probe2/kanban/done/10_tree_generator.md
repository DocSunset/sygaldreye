# tree_generator component

Procedurally generates a low-poly tree as a `TriMeshData` (trunk + branches as faceted cylinders) plus a list of `BillboardInstance` leaf clusters. Parameterised enough to produce jungle canopy, pine, dead winter tree, coral, lightning bolt, or alien growth.

## Approach

- `tree_generator.hpp` declares:
  ```cpp
  struct BranchParams {
      float length_scale   = 0.7f;  // child length relative to parent
      float radius_scale   = 0.6f;  // child radius relative to parent
      float spread_angle   = 0.4f;  // radians from parent axis
      int   branch_count   = 3;     // children per node
      int   max_depth      = 5;
      float angle_noise    = 0.3f;  // noise perturbation on spread angle
      uint32_t seed        = 42;
  };

  struct TreeParams {
      BranchParams branch;
      float trunk_height   = 4.0f;  // metres
      float trunk_radius   = 0.15f;
      int   cylinder_slices = 5;    // low-poly: 5 or 6 sides
      // Leaf clusters at branch tips
      bool  enable_leaves  = true;
      float leaf_cluster_radius = 0.5f;
      Eigen::Vector4f leaf_color = {0.2f, 0.6f, 0.15f, 1.0f};
      // Branch and trunk color
      Eigen::Vector4f bark_color = {0.35f, 0.22f, 0.1f, 1.0f};
  };

  struct TreeMesh {
      TriMeshData              branches;    // trunk + all branches
      std::vector<BillboardInstance> leaves;
  };

  TreeMesh generate_tree(TreeParams const&);
  ```
- Recursive branching: each node emits `branch_count` children rotated by `spread_angle` plus seeded random perturbation. Branch geometry is a tapered cylinder segment; normals are faceted (flat shading).
- Leaf clusters are `BillboardInstance` entries positioned at branch tips with depth ≥ `max_depth - 1`.
- `tree_generator.test.cpp`: verify branch count grows recursively, all normals unit length, leaf count matches tip count.

## Acceptance

- A default tree renders with visible trunk, branches, and leaf clusters in-headset
- `max_depth=2, branch_count=2` produces exactly 2+4=6 branches (plus trunk) — test-verifiable
- Host-buildable; tests pass
- `tree_generator.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `noise` (item 01) — angle perturbation
- `tri_mesh` (item 03)
- `billboard_quad` (item 09)
