# poke_stick

A narrow stick attached to a controller pose for near-field, collision-based
interaction (Travis 2026-06-11). Widgets test against `tip_pos` instead of a
ray; pointing stays for far-field. **First node shipped as a live plugin** —
see `kanban/backlog/live_update_gap.md`.

## Ports

- Inputs: `pos` (vec3), `rot` (quat) — controller pose; `length`, `radius`
  (sliders, metres); `active` (0/1 — highlights gold, driven by trigger or a
  future intersect node).
- Outputs: `tip_pos` (vec3) — stick tip in world space, the collision probe;
  `surface`+`mesh` (Mesh+Surface for a `draw` node — unlit uniform-color
  box, gold when active else grey).
- Sources: pose comes from `hand`/`controller` nodes via edges.
- Destinations: `tip_pos` feeds poke-aware widget nodes; `surface`/`mesh`
  feed a `draw` node.
- Temporal couplings: none (stateless per tick).
- Intended seams: collision testing deliberately lives OUTSIDE this node
  (intersect nodes edge tip_pos against widget bounds).

## Requirements

- ABI v8: render is a declarative Mesh+Surface, never GL. Process is pure
  (graphs parse on the HTTP thread); GL lives only in render_region.
- Self-contained so it works as a dlopen'd plugin.

## Allowed dependencies

sygaldry_endpoints, render_payloads, tri_mesh, eyeballs_node_abi (plugin
export only), Eigen.

## Owning package

scene
