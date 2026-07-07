# migrate_xr_sources

Create a new `xr_sources` component containing thin XR source nodes for HMD pose and controller state. Register them in `ComponentRegistry` so they appear in the palette and can be instantiated in the graph.

## New component

Create `components/xr_sources/` with:
- `xr_sources.hpp`
- `xr_sources.cpp` (can be minimal / empty impls)
- `CMakeLists.txt`

## Nodes to create

### HeadPoseNode

```cpp
struct HeadPoseNode {
    static consteval std::string_view name()          { return "head_pose"; }
    static consteval std::string_view source_header() { return "components/xr_sources/xr_sources.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/xr_sources/xr_sources.cpp"; }
    struct inputs {} inputs;
    // App calls set_pose() each frame before tick_graph
    void set_pose(const XrPosef& p) { pose_ = p; }
    XrPosef current_pose() const { return pose_; }
    void operator()(double) {}
private:
    XrPosef pose_{};
};
```

### LeftControllerNode

```cpp
struct LeftControllerNode {
    static consteval std::string_view name()          { return "left_controller"; }
    static consteval std::string_view source_header() { return "components/xr_sources/xr_sources.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/xr_sources/xr_sources.cpp"; }
    struct inputs {} inputs;
    void set_state(const XrPosef& p, bool trigger) { pose_ = p; trigger_ = trigger; }
    XrPosef pose() const { return pose_; }
    bool trigger_pressed() const { return trigger_; }
    void operator()(double) {}
private:
    XrPosef pose_{};
    bool    trigger_ = false;
};
```

### RightControllerNode

Identical to `LeftControllerNode` with `name()` returning `"right_controller"`.

## Notes on `inputs {}` and serialization

`to_json` / `from_json` via Boost.PFR will see an empty `inputs` struct and produce `{}`. That is fine.

`XrPosef` is not serialized by PFR (it's not in `inputs`). The pose state is internal — only the app pumps it via `set_pose()` / `set_state()` each frame. This is intentional: XR sources are data-push from the platform, not param-knobs.

## CMakeLists.txt for xr_sources

```cmake
add_library(xr_sources INTERFACE)
target_include_directories(xr_sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(xr_sources INTERFACE openxr_loader)
```

Use INTERFACE because the impl is header-only (all methods are inline).

## Registration in app.cpp

```cpp
#include "xr_sources.hpp"
// In android_main, after other register_builtin calls:
state.registry_.register_builtin(make_descriptor<HeadPoseNode>());
state.registry_.register_builtin(make_descriptor<LeftControllerNode>());
state.registry_.register_builtin(make_descriptor<RightControllerNode>());
```

Add `xr_sources` to `components/app/CMakeLists.txt` target_link_libraries.

## Future work (NOT part of this item)

- Phase 5 edges: expose `XrPosef` as typed output port so downstream nodes can consume it
- Feed actual pose data from `Input` into these nodes each frame (for now they're just palette entries with empty state)

## Constraints

- Warnings as errors
- Build must pass for Android (aarch64)
- `inputs` must be aggregate: no constructors, no private members
- Keep it header-only; no substantive .cpp needed
