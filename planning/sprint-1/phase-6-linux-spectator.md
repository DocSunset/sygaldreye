# Phase 6: Linux Spectator + Multi-user

## Goal

The Linux companion renders the same scene as the headset in a desktop window. Multiple
people on the LAN can connect, watch, and contribute parameter and graph changes in real
time. The headset is the authoritative state; all clients converge to it.

## Deliverables

### Linux renderer

- `host_gl_context` already exists (EGL on Linux, used for tests). Extend it to drive
  a persistent render loop at display refresh rate.
- Companion discovers headset via mDNS (Phase 2), subscribes to param/graph state.
- All node types that compile for the host (i.e., don't use `XR_KHR_opengl_es_enable`
  directly) run identically on Linux. This is most scene/audio nodes.
- View/projection matrix: companion uses a free-camera (mouse/keyboard) or a fixed
  overview camera; it does not receive XR eye poses (it has no headset).
- Draw output: desktop OpenGL window (EGL + platform window, or SDL/GLFW for the window
  handle).

### State synchronization

The headset broadcasts the current `GET /params` and `GET /graph` responses to all
connected companions whenever either changes. Broadcast is HTTP server-sent events or
periodic polling — SSE is preferred (push, low overhead, text/event-stream).

Friends connect by pointing their companion at the headset's mDNS-advertised address.
They POST `/params` or `/graph` to contribute changes. Last-write-wins.

### Plugin sync

If a friend uploads a plugin (POST `/plugins` to the headset), the headset re-broadcasts
the updated registry. Other companions can optionally fetch and load the same `.so` if
they want to render the new node type locally.

## Key Design Decisions

**Companions are thin clients, not peers.** The headset holds the graph. Companions render
a local copy but do not need consensus; brief divergence during a POST round-trip is
acceptable. This is an art tool, not a synchronized multiplayer simulation.

**XR-specific nodes degrade gracefully on the host.** `head_pose` and controller source
nodes can emit a neutral/identity pose on Linux (or accept keyboard/mouse input as a
substitute). The graph still evaluates; non-XR nodes render normally.

**No cross-compilation required for Linux.** The companion runs on Linux natively.
Node `.so` plugins compiled for Android (ARM64) cannot run on the companion. The companion
should recompile any uploaded plugin source for x86_64 as well, if the source is available.

## Dependencies

Phase 5 (universal signal graph) — companion needs the graph to know what to render.
Phase 2 (networking) — mDNS discovery, HTTP broadcast.
