# Node: `ptt_gate`

## Goal

A PD-style gate node: passes an `AudioBuffer` through while a control input is held
open, and emits bang signals on open and close events. In the push-to-talk chain:

```
[left_controller.trigger] → [trigger_edge.press / .release]
                                        ↓               ↓
[mic_input.audio_out] → [ptt_gate.audio_in]         open/close
                              ↓
                        [ptt_gate.audio_out] → [speech_to_text.audio_in]
[ptt_gate.opened]                            → [speech_to_text.begin]
[ptt_gate.closed]                            → [speech_to_text.finalize]
```

The gate opens on an `open` bang (> 0.5), closes on a `close` bang (> 0.5).
While open it passes audio through; while closed it outputs an empty AudioBuffer.

## Feasibility Assessment

**Straightforward.** The node is essentially a stateful switch: `bool open_` tracks
gate state, toggled by bang inputs. AudioBuffer pass-through is a pointer copy — no
audio data is copied, just the struct.

One subtlety: `open` and `close` bangs can arrive in the same tick (if trigger_edge
fires both simultaneously — impossible by definition since it's edge-triggered). Not
a concern in practice; handle defensively: process `close` after `open` in the same
tick if both arrive (last write wins).

`AudioBuffer` holds a borrowed pointer valid for the current tick. The output
`audio_out` simply copies the struct when open (same borrowed pointer; downstream
node consumes it in the same tick). When closed, output `{nullptr, 0, 1, 16000}`.

The gate also emits `opened` (1.0) and `closed` (1.0) float bangs on state
transitions, so downstream nodes can react without needing their own edge detection.

## Component

`components/ptt_gate/`

```
ptt_gate/
  ptt_gate.hpp
  ptt_gate.cpp
  ptt_gate.test.cpp
  CMakeLists.txt
```

## Design

```cpp
class PttGate {
public:
    static consteval std::string_view name() { return "ptt_gate"; }

    struct inputs {
        port<"audio_in", AudioBuffer> audio_in;
        port<"open",     float>       open;   // bang: > 0.5 opens gate
        port<"close",    float>       close;  // bang: > 0.5 closes gate
    } inputs;

    struct outputs {
        port<"audio_out", AudioBuffer> audio_out;
        port<"opened",    float>       opened; // 1.0 on the tick gate opens
        port<"closed",    float>       closed; // 1.0 on the tick gate closes
        port<"is_open",   float>       is_open; // 1.0 while gate is open
    } outputs;

    void operator()(double time_s);

private:
    bool open_ = false;
};
```

`operator()`:
```cpp
bool was_open = open_;
if (inputs.open.value  > 0.5f) open_ = true;
if (inputs.close.value > 0.5f) open_ = false;

outputs.opened.value   = (!was_open &&  open_) ? 1.f : 0.f;
outputs.closed.value   = ( was_open && !open_) ? 1.f : 0.f;
outputs.is_open.value  = open_ ? 1.f : 0.f;
outputs.audio_out.value = open_ ? inputs.audio_in.value
                                 : AudioBuffer{nullptr, 0, 1, 16000};
```

## Registration

```cpp
#include "ptt_gate.hpp"
state.registry_.register_builtin(make_descriptor<PttGate>());
```

## Test

`ptt_gate.test.cpp`:
- Gate starts closed; audio_out.frames == 0
- After open bang: is_open = 1, opened = 1, audio_out passes through
- opened = 0 on subsequent ticks (not latched)
- After close bang: closed = 1, is_open = 0, audio_out is empty
- closed = 0 on subsequent ticks

## Acceptance Criteria

- Node compiles, registers, and passes unit tests
- `sh/build.sh` passes
