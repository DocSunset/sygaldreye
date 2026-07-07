# THE gap: arbitrary live updates via graph + plugins (vision check)

Travis (2026-06-11): every UX complaint this session pattern-matched to
"edit the C++ and reinstall" — the measure of how far the graph/plugin
model still falls short. Target: arbitrary updates to almost any part of
the RUNNING app via graph edits and shipped plugin .so/.json only.

RATIFIED as the guiding star (vision.md "Guiding star"): **every restart
needed to land a change is a failed test revealing a bug.** Standing
question for every such restart: "what would need to have been changed
before so this change could land without restarting?" — file the answer
here. Companion + spectator apps are eventually subgraphs too.

## Restart-failure log (the virtuous consequence)

- 2026-06-11 aim-pose ray fix (input.cpp rebind) needed reinstall.
  Answer: pose binding should be a node param / graph-level choice —
  the xr input rig belongs in the graph (executor design, blocker 5).
- 2026-06-11 slider display seeding + nearest-track hit test
  (vr_editor) needed restart. Answer: blocker 1 — widgets as graph
  nodes would have made both a plugin replace + graph edit.
- 2026-06-11 host /plugins route itself had to be ADDED by restart.
  Answer: blocker 5/unification — the HTTP surface is part of the
  core spine, currently duplicated per shell.
- 2026-06-12 ergonomics batch (ray removal, text halving, palette
  paging, card-grid clamp) needed one APK reinstall. Answers: text
  scale is now a live editor param (gap closed for that one — only the
  DEFAULT is baked); the rest is blocker 1 again — ray/palette/card
  layout live in the vr_editor monolith, and blocker "device plugins
  crash" (device_plugin_crash.md) blocks the hot path that would have
  shipped this without restart. Fixing the device dlopen crash IS the
  ergonomics-iteration enabler.
- 2026-06-12 wire-draw framerate fix (editor_wire_batching.md) cannot
  land as a graph edit: wire VISUALS are VrEditor C++ (blocker 1), and
  there is no payload type for "a bunch of vertices" anyway. Answer
  (Travis): a dynamic-array endpoint payload — a span/buffer view like
  AudioBuffer but generic — so the graph's edge list and a wire mesh
  are both portable data, and the wire renderer becomes a node
  (edge list in → mesh out → draw). Folding the span payload into
  endpoints v6; wire-renderer-as-node is the blocker-1 slice after.
- 2026-06-12 UNIFICATION LANDED: components/peer_core owns registry,
  graph swap+migrate, queue mappings, the ENTIRE HTTP surface, values
  snapshot (fixes a device /values data race), screenshot fulfilment.
  host_app and app.cpp are thin shells (node list + default graph +
  frame pump). Host gains GET /screenshot, /meta-graph, SSE broadcast;
  device gains /camera /controller sugar, rich /values, /quit.
  Remaining shell divergence (intentional for now): device drives
  VrEditor directly instead of the editor node (flip the device default
  graph after on-device verify); assets/graphs not packaged on device
  (graphs_dir empty there).

Current blocker inventory:
1. The C++ vr_editor monolith still owns cards/wires/sliders/palette.
   FIRST SLICE CARVED 2026-06-12: poke_stick + poke_button +
   spawner.spawn (bang) = collision-based interaction fully in the
   graph — poking a button in space spawned a cube into the running
   graph with zero editor C++. Both nodes are plugin-shippable
   (SYGALDREYE_PLUGIN). Next slices: poke-able sliders/knobs, wire
   handles, cards.
2. ✅ HOST-PROVEN 2026-06-11: poke_stick compiled (--target host),
   POSTed to /plugins on the RUNNING spectator, dlopen'd, wired to
   hand_r by live graph edit; renders + reacts to trigger. Restart
   failures found en route, both fixed: compile_node.py include list
   had drifted (missing gpu_texture/tri_mesh — answer: plugin include
   set must come from the build system, not a hand-copied list), and
   host_app had NO /plugins route (answer: core unification). Android
   replay still pending (NDK flags unverified).
3. ✅ HOT-RELOAD LANDED 2026-06-12 (host-proven, device replay pending):
   re-register same name → old entry retires (handle NEVER dlclosed
   while registry lives — live instances run old code until swap);
   POST /plugins auto-queues a re-parse of the running graph → reloaded
   type gets fresh instances with params carried via serialization,
   untouched types adopt state via migration. Gotcha burned in tests:
   plugins MUST compile with -fno-gnu-unique or glibc unifies the
   descriptor static process-wide and the reload silently no-ops.
   Internal (non-param) state resets on reload — acceptable v1.
4. assets/graphs/*.json not packaged/loadable on device (no sky/aurora/
   orbit_cam types there) — subgraph plugins are host-only in practice.
5. Core spine (renderer, frame loop, executor, audio) not graph-
   expressed — executor design (planning/edge_executor.design.md) is
   the long-term answer; mappings/regions must land before "any part".

Next-step candidate (proposed): prove the loop on a real need — ship
poke_stick interaction as a live plugin to the running headset
(poke_stick_interaction.md). It exercises 2 (pipeline), surfaces 3
(reload on iteration), and chips at 1 (UI from nodes instead of
monolith).

- 2026-06-12 BLOCKER CLEARED: device plugin dlopen verified end-to-end
  (compile → upload → hot reload → spawned instance ticking → stable
  across swaps). The hot path for shipping editor slices / node fixes
  to the RUNNING headset without reinstall is now open on-device, not
  just host.
