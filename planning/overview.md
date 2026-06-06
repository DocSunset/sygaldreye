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

### Multi-rate evaluation

Nodes declare the rate at which they operate. The graph evaluates each rate domain
independently on the appropriate trigger.

| Rate    | Trigger           | Port types                          |
|---------|-------------------|-------------------------------------|
| Audio   | Audio callback    | `audio_buffer`                      |
| Frame   | XR frame loop     | `XrPosef`, `vec3`, `texture`, `draw_call` |
| Control | On-change / slow  | `float`, `bool`, `bang`             |

Edges crossing rate boundaries require explicit rate-conversion nodes (analogous to
Pd's `snapshot~` / `sig~`).

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

| Type            | Rate          | Description                              |
|-----------------|---------------|------------------------------------------|
| `slider<>`      | control/frame | Scalar float with min/max/init           |
| `toggle<>`      | control/frame | Boolean                                  |
| `bang<>`        | any           | Event / impulse                          |
| `XrPosef`       | frame         | 6DOF pose (position + orientation)       |
| `audio_buffer`  | audio         | Block of PCM float samples               |
| `texture`       | frame         | GPU texture handle + dimensions + format |
| `draw_call`     | frame         | Renderable geometry → compositor         |

## Phase Summary

| Phase | Title                        | Key deliverable                                  |
|-------|------------------------------|--------------------------------------------------|
| 1     | Param registry               | PFR + sygaldry endpoints + JSON serialization    |
| 2     | Networking                   | mDNS, HTTP server, Linux companion               |
| 3     | Speech-to-text               | AAudio mic → companion → Whisper → commands      |
| 4     | Plugin ABI                   | C descriptor, dlopen, cross-compile pipeline     |
| 5     | Universal signal graph       | Multi-rate runtime, XR sources, wireable poses   |
| 6     | Linux spectator + multi-user | host_gl_context mirror, collaborative control    |
| 7     | In-VR structural editor      | Node palette, cards, port handles, wire rendering|
| 8     | GPU texture ports             | FBO scheduler, texture edge type, render graph   |
