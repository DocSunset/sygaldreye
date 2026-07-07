# WI-5: RendererNode

## Summary
Create a new built-in node `RendererNode` with 14 eye-pose override input ports so
VR editors can wire controller or head pose into the renderer's eye positions.

## Work

Create `components/renderer_node/` with:
- `renderer_node.hpp`
- `renderer_node.cpp`
- `CMakeLists.txt`

### renderer_node.hpp
```cpp
#pragma once
#include "sygaldry_endpoints.hpp"
#include <string_view>

struct RendererNode {
    static consteval std::string_view name()          { return "renderer"; }
    static consteval std::string_view source_header() { return "components/renderer_node/renderer_node.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/renderer_node/renderer_node.cpp"; }

    struct inputs {
        // Use slider<> with fp() for VR editor range metadata
        slider<"left_pos_x",  "", float, fp(-5.f), fp(5.f), fp(0.f)> left_pos_x;
        slider<"left_pos_y",  "", float, fp(-5.f), fp(5.f), fp(0.f)> left_pos_y;
        slider<"left_pos_z",  "", float, fp(-5.f), fp(5.f), fp(0.f)> left_pos_z;
        slider<"left_rot_x",  "", float, fp(-1.f), fp(1.f), fp(0.f)> left_rot_x;
        slider<"left_rot_y",  "", float, fp(-1.f), fp(1.f), fp(0.f)> left_rot_y;
        slider<"left_rot_z",  "", float, fp(-1.f), fp(1.f), fp(0.f)> left_rot_z;
        slider<"left_rot_w",  "", float, fp(-1.f), fp(1.f), fp(1.f)> left_rot_w;
        slider<"right_pos_x", "", float, fp(-5.f), fp(5.f), fp(0.f)> right_pos_x;
        slider<"right_pos_y", "", float, fp(-5.f), fp(5.f), fp(0.f)> right_pos_y;
        slider<"right_pos_z", "", float, fp(-5.f), fp(5.f), fp(0.f)> right_pos_z;
        slider<"right_rot_x", "", float, fp(-1.f), fp(1.f), fp(0.f)> right_rot_x;
        slider<"right_rot_y", "", float, fp(-1.f), fp(1.f), fp(0.f)> right_rot_y;
        slider<"right_rot_z", "", float, fp(-1.f), fp(1.f), fp(0.f)> right_rot_z;
        slider<"right_rot_w", "", float, fp(-1.f), fp(1.f), fp(1.f)> right_rot_w;
    } inputs;
    struct outputs {} outputs;
    void operator()(double) {}
};
```

### CMakeLists.txt
Library linking `sygaldry_endpoints` and `eyeballs_node_abi`.

### Root CMakeLists.txt
Add `add_subdirectory(components/renderer_node)` and link to `eyeballs`.

### app.cpp
- `#include "renderer_node.hpp"`
- `state.registry_.register_builtin(make_descriptor<RendererNode>());`
- Update `kDefaultGraph` JSON to include renderer node

## Acceptance
- `RendererNode` registered in app
- Default graph JSON includes it
- `sh/build.sh` passes
- Commit
