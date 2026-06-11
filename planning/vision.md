# Vision — Sygaldreye

_Source: conversation with Travis, 2026-06-10. This document is the spine.
Future agents: read this first. Keep `planning/status.md` current as you work._

## The dream

A VR environment where **everything** — scene content, rendering, camera,
the editor UI itself — is editable live through a rich, generic graph
execution model. Patch nodes at runtime like a modular synthesizer for
audio and procedural VR graphics.

**Networked first-class**: the app runs simultaneously on desktop, Quest 3,
and phone (browser/WASM peer). Nodes sourcing hardware-specific data
(controllers, IMU) can be instantiated on *other* devices and receive data
over the network invisibly. Remote nodes "just work".

**LLM-native**: some instances have LLM coding assistants that write new
nodes in C++, cross-compile them for other instances, and ship them over
the network for live loading — no restart.

## Guiding star (Travis, 2026-06-11)

**Any time we must restart the app to change or extend it — new feature,
bugfix, anything — we are failing a crucial test and revealing a bug.**
Always ask: *"what would need to have been changed before, so that this
change could land without restarting?"* and fix that.

Long term, the graph execution model consumes everything, **including
itself**: everything we want to do is expressible as a subgraph over a
finely curated set of fundamental node types, and anything else drops into
the running app as a plugin. The companion and spectator apps are *just
more subgraphs* — expressed through, and hackable via, the same graph
execution model. "Everything we want to express should be possible to
express as a subgraph."

Measuring stick for new fundamental node types: minimize duplication;
require modularity and separation of concerns; the usual reusable,
maintainable, flexible, changeable design virtues. Prefer composition of
existing nodes over a new primitive.

## Guiding principles

- **Express almost everything through the graph engine.** Core
  infrastructure (networking, registry, executors' wiring) should itself be
  graph-expressible where possible. Higher-level functionality is assembled
  as *reusable subgraph compositions*. Target: the node graph becomes a
  flexible, complete interpreted programming language unto itself.
- **Small nodes.** Existing nodes are excessively coarse; decompose into
  subgraphs of many small parts. The 2026-06-06 consolidation study
  (`studies/component-consolidation.md`) is *provisional evidence*, not the
  plan — redo it once the edge model lands.
- **What is an edge?** is the load-bearing design question. Edge kinds
  (latest-value, lossless event, ordered stream, block-rate audio,
  predicted/posed, ...) must be *discovered from real use*, not ratified
  upfront. Travis ratifies the edge/executor design when it's written.
- **Agents are peers, not backdoors.** LLM control (look, move, click)
  flows through the same source-node edges as mouse/keyboard. Every bug an
  agent can reproduce, a human can, and vice versa.
- **Iterate via JSON over the network.** Agents speak JSON fluently; hit
  the scene-graph overwrite endpoint to edit the live graph instead of
  rebuilding C++. Prefer adding graph JSON over adding C++.
- **Browser peer constraints apply now**: network bridge protocol needs a
  WebSocket-capable transport from day one; keep host platform seams
  (window, GL context, audio, executor bootstrap) clean — the Emscripten
  target reuses them. New-nodes-over-network on web = WASM side modules.

## Slice plan

1. **Agent-operable host app** — spectator app on Linux; mouse/keyboard +
   agent-drivable camera/interaction source nodes; generic probe /
   texture-tap / audio-tap nodes over HTTP; `sh/agent/` script kit
   (look/move/click/screenshot/probe). Ends: agent can launch, fly, operate
   the editor, capture evidence.
2. **Bug hunt** — exercise the editor via those nodes on host, then Quest.
   Expect app-breaking bugs; most fixes land in shared code.
3. **Edge/executor design** — design doc; edge kinds + execution regions
   (render/audio/network), from slice 1–2 evidence. Travis signs off.
4. **Network bridge** — cross-region edge over WebSocket-capable transport;
   proxy nodes generated from descriptor metadata → remote nodes just work.
5. **Decomposition** — redo consolidation study against the edge model;
   split coarse nodes into subgraphs.
6. **Browser peer** — Emscripten target on the host seams; WebGL2,
   AudioWorklet, WASM side modules over the network.
7. **LLM codegen loop** — cross-compile + ship + live-load, building on
   `companion/` pipeline.

## Working agreements

- Travis is mostly away from his desk. Send Telegram updates as work
  progresses, including snapshots/screenshots the agent itself sees.
- Keep `planning/status.md` updated often — current state, what's next,
  surprises. Write notes to the filesystem liberally.
- Audio inspection: agents can't hear; audio taps emit spectrogram/waveform
  PNGs + numeric features (RMS, FFT peaks, onsets).
- Snapshot system (`scene_snapshot`, bespoke per-node files) is deprecated
  in spirit: replace with generic in-graph probe/tap nodes.
- Project standards in CLAUDE.md apply: brevity, less code, design docs,
  components, kanban workflow.
