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
  `render` (draw_call).
- Sources: pose comes from `hand`/`controller` nodes via edges.
- Destinations: `tip_pos` feeds future poke-aware widget nodes.
- Temporal couplings: none (stateless per tick besides lazy GL).
- Intended seams: collision testing deliberately lives OUTSIDE this node
  (intersect nodes edge tip_pos against widget bounds).

## Requirements

- Constructor cheap and GL-free (graphs parse on the HTTP thread); GL init
  lazily inside the DrawFn on the render thread.
- Self-contained GL (own shader/VAO) so it works as a dlopen'd plugin.

## Allowed dependencies

sygaldry_endpoints, eyeballs_node_abi (plugin export only), Eigen, GLES3.

## Owning package

scene
