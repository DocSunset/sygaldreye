# scene_graph component

A lightweight transform hierarchy for composing multi-part objects: trees with branches, city blocks with buildings, space stations with solar panels. Each node holds a local transform and a list of child indices; the scene graph resolves world transforms by walking the tree.

## Approach

- `scene_graph.hpp` declares:
  ```cpp
  using NodeId = uint32_t;
  constexpr NodeId k_null_node = UINT32_MAX;

  struct Node {
      Eigen::Affine3f local_transform = Eigen::Affine3f::Identity();
      NodeId          parent          = k_null_node;
  };

  class SceneGraph {
  public:
      NodeId  add_node(NodeId parent = k_null_node);
      void    set_transform(NodeId, Eigen::Affine3f const&);
      Eigen::Matrix4f world_transform(NodeId) const; // walks to root, caches
      void    invalidate(NodeId);   // mark subtree dirty when local changes
  };
  ```
- Node storage: `std::vector<Node>` with indices as stable IDs (nodes are never removed mid-session in practice, simplifying the implementation).
- World transform cache: a parallel `std::vector<std::optional<Eigen::Matrix4f>>` cleared by `invalidate()`. Computed lazily on first `world_transform()` call per frame.
- No rendering logic — purely transform math. Renderable objects hold a `NodeId` and query the graph for their MVP.
- `scene_graph.test.cpp`: verify parent-child transform composition, verify cache invalidation propagates to children, verify identity default.

## Acceptance

- A three-level hierarchy (root → branch → leaf) computes the correct composed world transform (test-verifiable with known matrices)
- Host-buildable; tests pass; no GL or XR dependencies
- `scene_graph.design.md` present; `.cpp` under 100 lines of substantive code
