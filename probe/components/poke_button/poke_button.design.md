# poke_button

Collision-based UI as a graph node — the first slice of the editor
recomposition (live_update_gap blocker #1). A button in space fires when a
poke probe is inside it and the press input rises; wiring its bang into a
spawner makes the graph grow itself with zero editor C++.

## Ports

- Inputs: `tip` (vec3, the probe — a poke_stick tip); `press` (0/1,
  trigger); `x`/`y`/`z`/`size` (placement, declarative params).
- Outputs: `pressed` (bang — tip inside AND press rising); `hover`
  (1 while inside); `surface`+`mesh` (Mesh+Surface for a `draw` node —
  unlit uniform-color box; blue idle, light-blue hover, gold fire).
- Destinations: `surface`/`mesh` feed a `draw` node; `pressed`/`hover`
  feed any scalar/bang consumer (spawner.trigger).
- Intended seams: collision shape is an axis-aligned cube v1; richer
  shapes become sibling nodes, not options.

## Requirements

- ABI v8: render is a declarative Mesh+Surface, never GL. Process is pure;
  GL lives only in render_region.
- Self-contained so it ships as a plugin (SYGALDREYE_PLUGIN export).

## Allowed dependencies

sygaldry_endpoints, render_payloads, tri_mesh, eyeballs_node_abi (plugin
export only), Eigen.

## Owning package

scene
