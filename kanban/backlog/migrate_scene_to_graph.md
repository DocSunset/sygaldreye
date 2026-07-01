# Migrate Scene behavior to the node graph, then remove Scene

`components/scene/scene.cpp` implements three behaviors that predate the node
graph and are not yet expressed in it:

1. **Animated rotating world cube** — a blue cube at (0, 1.5, -5) that rotates
   with independent sinusoidal speeds on each axis over time.
2. **Controller-following hand cubes** — small cubes (scale 0.035) that track
   the live `XrPosef` of each hand; red for left, green for right.
3. **Face labels on the right controller cube** — four text labels ("toward
   user", "forward", "top", "bottom") positioned on the faces of the right hand
   cube.

Currently `scene_.update()` and `scene_.set_controller_poses()` are called in
the frame loop but `scene_.cubes()` and `scene_.labels()` are never passed to
the renderer — so **none of this is rendered**. Before removing `Scene`,
reproduce these behaviors in the graph:

- Extend `CubeNode` (or create a new node) to accept rotation inputs so
  controller poses (pos + quaternion) from `ControllerNode` outputs can drive a
  cube's full 6-DOF transform.
- Create a time node that just outputs the time since its first tick in seconds
  as a float
- Create a RGBA color endpoint type
- Express the time-animated world cube as a wiring of new and existing nodes
- Decide whether the face labels are worth keeping; if so, express them via
  `TextLabelNode` or a new node.

Once the graph replicates the behavior, remove `Scene scene_` from `AppState`,
the `scene_.update()` / `scene_.set_controller_poses()` calls, and the
`#include "scene.hpp"`.
