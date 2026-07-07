# WI-8: App.cpp integration

## Summary
Remove hard-wired scene members; draw from `graph.draw_calls`; pump XR sources
before tick_graph; apply RendererNode eye-pose overrides; update default graph JSON.

## Work

### A. Remove hard-wired members from AppState
- Remove `water_`, `sky_`, `rd_gpu_` members
- Remove corresponding init code (WaterSurface::create, SkyDome::create, rd_gpu_.init)
- Remove direct draw calls for water and sky in the render callback
- Keep `cube_mesh_`, `text_mesh_`, `scene_`, VR editor — they are not in the graph

### B. Replace draw calls in render callback
Find where app calls `sky_->draw()` and `water_->draw()`. Replace with:
```cpp
if (active_graph_) {
    for (auto& call : active_graph_->draw_calls)
        call(pv);  // pv = proj * view
}
```

### C. Pump XR source nodes before tick_graph
```cpp
if (state.active_graph_) {
    for (auto& n : state.active_graph_->nodes) {
        if (std::string_view(n.desc->type_name) == "head_pose")
            static_cast<HeadPoseNode*>(n.data)->set_pose(hmd_pose);
        else if (std::string_view(n.desc->type_name) == "left_controller")
            static_cast<LeftControllerNode*>(n.data)->set_state(lh_pose, trigger_left, ...);
        else if (std::string_view(n.desc->type_name) == "right_controller")
            static_cast<RightControllerNode*>(n.data)->set_state(rh_pose, trigger_right, ...);
    }
    tick_graph(*state.active_graph_, time_sec);
}
```

Find where the app gets hmd_pose — it must come from XrLocateViews or similar.
For now, if hmd pose isn't directly available before render, use a zero pose or skip.

### D. RendererNode eye-pose overrides
After tick_graph, check if any edge targets renderer's left_* or right_* ports:
```cpp
for (const auto& n : state.active_graph_->nodes) {
    if (std::string_view(n.desc->type_name) == "renderer") {
        bool left_wired = false, right_wired = false;
        for (const auto& e : state.active_graph_->edges) {
            if (e.to_node == n.id) {
                if (e.to_port.rfind("left_", 0) == 0)  left_wired  = true;
                if (e.to_port.rfind("right_", 0) == 0) right_wired = true;
            }
        }
        // Store override flags on AppState for use in render callback
    }
}
```
The renderer currently uses `views[eye].pose` from xrLocateViews. Override TBD if
architecture permits; for now, record that overrides are wired.

### E. Default startup graph
Replace `kDefaultGraph` with sky+water+head+renderer nodes and one edge:
```json
{"nodes":[
  {"id":"sky","type":"sky_dome","params":{"sun elevation":0.3}},
  {"id":"water","type":"water_surface","params":{"cell size":1.0}},
  {"id":"head","type":"head_pose","params":{}},
  {"id":"renderer","type":"renderer","params":{}}
],"edges":[
  {"from":"sky.sun_elevation_out","to":"water.sun elevation"}
]}
```

### F. Include headers
Add `#include "renderer_node.hpp"` and keep `#include "xr_sources.hpp"`.

### G. Register RendererNode
`state.registry_.register_builtin(make_descriptor<RendererNode>());`

## Acceptance
- No direct `sky_->draw()` or `water_->draw()` calls
- `graph.draw_calls` drives rendering
- XR source nodes pumped each frame
- RendererNode registered
- `sh/build.sh` passes
- Commit
