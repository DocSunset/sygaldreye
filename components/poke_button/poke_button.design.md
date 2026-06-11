# poke_button

Collision-based UI as a graph node — the first slice of the editor
recomposition (live_update_gap blocker #1). A button in space fires when a
poke probe is inside it and the press input rises; wiring its bang into a
spawner makes the graph grow itself with zero editor C++.

## Ports

- Inputs: `tip` (vec3, the probe — a poke_stick tip); `press` (0/1,
  trigger); `x`/`y`/`z`/`size` (placement, declarative params).
- Outputs: `pressed` (bang — tip inside AND press rising); `hover`
  (1 while inside); `render` (draw_call — blue idle, light-blue hover,
  gold fire).
- Intended seams: collision shape is an axis-aligned cube v1; richer
  shapes become sibling nodes, not options.

## Requirements

- Constructor cheap/GL-free; GL lazily inside the DrawFn (render thread).
- Self-contained GL so it ships as a plugin (SYGALDREYE_PLUGIN export).

## Allowed dependencies

sygaldry_endpoints, eyeballs_node_abi (plugin export only), Eigen, GLES3.

## Owning package

scene
