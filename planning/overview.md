# Eyeballs: Architecture Overview

## Vision

Everything is a node. Audio samples, controller poses, eye positions, microphone captures,
button states, rendered geometry, simulation textures — all typed signals flowing through a
universal routing fabric. The boundary between audio engine, visual scene, and XR input stack
is an artifact of conventional architecture; here they are all nodes in the same graph.

The namesake: by exposing the renderer's eye-pose input as a wireable port, you can reroute
head tracking to a controller and hold your eyeballs in your fist. Body schema manipulation
through signal routing.

An LLM assistant can write a new node type, cross-compile it, and POST the `.so` to the
headset — the headset loads it immediately without restarting. Friends on the LAN can connect
to the same headset and contribute parameter changes in real time. You can issue voice commands
that the companion transcribes and routes through Claude to generate a new scene graph, which
it curls to the headset live.

## Architectural Principles

### Port declarations (sygaldry style)

Every node declares its ports as aggregate structs. Metadata lives in the type system.
PFR iterates these structs automatically — no manual registration.

```cpp
struct WaterSurface {
    struct inputs {
        slider<"wave height", "", float, 0.f, 20.f, 5.f> wave_height;
        slider<"wave speed",  "", float, 0.1f, 5.f, 1.f> wave_speed;
        toggle<"choppy">                                  choppy;
    } inputs;
    struct outputs {
        draw_call<"surface"> surface;
    } outputs;
    void operator()(double time);
};
```

Sygaldry is included as a git submodule for its metadata helper types only
(`string_literal`, `num_literal`, `range_`, `persistent`, `occasional`, and the endpoint
templates). Boost.PFR provides struct iteration and field-name reflection.

### Push/pull evaluation model

Nodes do not declare a rate. Rate emerges from the pull chain:

- **Sinks pull at their own rate.** `audio_output` pulls its upstream at audio rate
  inside the audio callback. `renderer` pulls its upstream at frame rate inside the XR
  frame loop. Rate propagates backwards through the graph from each sink.
- **Sources push when they have data.** Button presses, bangs, control changes propagate
  forward through the graph when they fire.
- **Most nodes are agnostic.** A filter, a transform, a gain node — they compute output
  from input and do not know or care what rate they are called at.

Rate is implicit in the port type: `audio_buffer` ports operate at audio rate; all other
types operate at frame or control rate. Connecting an `audio_buffer` output to a `float`
input is a type mismatch, rejected at connection time. Crossing the boundary requires an
explicit rate-conversion node (analogous to Pd's `snapshot~` / `sig~`).

### Stable C plugin ABI

Every node type — including built-ins — satisfies a C-linkage descriptor. This enables
dynamic loading via `dlopen()`, LLM-authored nodes compiled and uploaded over WiFi, and
hot-loading without restarting the app.

```c
typedef struct {
    const char* type_name;
    void*       (*create)();
    void        (*destroy)(void*);
    void        (*process)(void*, double time);
    const char* (*serialize)(void*);
    void        (*deserialize)(void*, const char*);
} EyeballsNodeDescriptor;

extern "C" const EyeballsNodeDescriptor* eyeballs_node_descriptor();
```

A macro/template generates this from any sygaldry-style C++ node automatically.

### Zero-configuration networking

The headset and Linux companion find each other via mDNS (mdns.h, no daemon dependency).

| Service                          | Advertised by   | Port |
|----------------------------------|-----------------|------|
| `_eyeballs._tcp.local.`          | Headset         | 8080 |
| `_eyeballs-companion._tcp.local.`| Linux companion | 9090 |

Android requires a `WifiManager.MulticastLock` (acquired via JNI) to receive multicast
packets.

### HTTP API surface (headset)

| Endpoint      | Method    | Purpose                                      |
|---------------|-----------|----------------------------------------------|
| `/params`     | GET, POST | Read / write scalar parameters               |
| `/graph`      | GET, POST | Read / replace the full runtime node graph   |
| `/plugins`    | POST      | Upload a compiled `.so`; headset dlopen()s it|
| `/meta-graph` | POST      | Out-of-band: reconfigure the editor UI itself|

## Port Type Lattice

Rate is not declared; it is implicit in the port type. `audio_buffer` is the only
audio-rate type; all others are frame/control rate. Connecting across this boundary
requires an explicit rate-conversion node.

| Type            | Description                              |
|-----------------|------------------------------------------|
| `slider<>`      | Scalar float with min/max/init           |
| `toggle<>`      | Boolean                                  |
| `bang<>`        | Event / impulse                          |
| `XrPosef`       | 6DOF pose (position + orientation)       |
| `audio_buffer`  | Block of PCM float samples (audio rate)  |
| `texture`       | GPU texture handle + dimensions + format |
| `draw_call`     | Renderable geometry → compositor         |

## Phase Summary

| Phase | Title                        | Key deliverable                                       |
|-------|------------------------------|-------------------------------------------------------|
| 1     | Param registry               | PFR + sygaldry endpoints + JSON serialization         |
| 2     | Networking                   | mDNS, HTTP server, Linux companion                    |
| 3     | Speech-to-text               | AAudio mic → companion → Whisper → commands           |
| 4     | Plugin ABI                   | Reflection-generated C descriptor, dlopen, cross-compile |
| 5     | Universal signal graph       | Push/pull runtime, XR sources, wireable poses         |
| 6     | Linux spectator + multi-user | host_gl_context mirror, collaborative control         |
| 7     | In-VR structural editor      | Node palette, cards, port handles, wire rendering     |
| 8     | GPU texture ports            | FBO scheduler, texture edge type, render graph        |
| 9     | Static graph compilation     | Runtime graph → generated C++ → optimised static binary |
