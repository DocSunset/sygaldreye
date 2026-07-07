# Node: `trigger_edge`

## Goal

Detect rising and falling edges on a float trigger input (e.g. from
`left_controller.trigger` or `right_controller.trigger`) and emit float "bang"
signals on press and release. This is the "controller events node" that replaces
the hardcoded edge-detection logic in `app.cpp` (lines 326–335,
`state.prev_trigger_left_`).

## Feasibility Assessment

**Trivial.** Pure stateful computation: one float of internal state (`prev_`),
two float outputs. No GL, no XR, no threading, no allocations.

The graph has no `bang` type in `PortValue`. Use `float` convention: 1.0 on the tick
when the event fires, 0.0 otherwise. Downstream nodes (ptt_gate, speech_to_text) treat
input > 0.5 as "bang received this tick". Document this convention in the port schema.

Threshold for "pressed": input > 0.5 (same convention the existing code uses implicitly
via `bool trigger_pressed()`).

## Component

`components/trigger_edge/`

```
trigger_edge/
  trigger_edge.hpp
  trigger_edge.cpp
  trigger_edge.test.cpp
  CMakeLists.txt
```

## Design

```cpp
class TriggerEdge {
public:
    static consteval std::string_view name() { return "trigger_edge"; }

    struct inputs {
        port<"trigger", float> trigger;
        slider<"threshold", "", float, fp(0.f), fp(1.f), fp(0.5f)> threshold;
    } inputs;

    struct outputs {
        port<"press",   float> press;   // 1.0 on rising edge, else 0.0
        port<"release", float> release; // 1.0 on falling edge, else 0.0
        port<"held",    float> held;    // 1.0 while trigger > threshold
    } outputs;

    void operator()(double time_s);

private:
    float prev_ = 0.f;
};
```

`operator()`:
```cpp
float t   = inputs.trigger.value;
float thr = inputs.threshold.value;
bool  cur = t > thr, was = prev_ > thr;
outputs.press.value   = (!was && cur)  ? 1.f : 0.f;
outputs.release.value = (was  && !cur) ? 1.f : 0.f;
outputs.held.value    = cur ? 1.f : 0.f;
prev_ = t;
```

## App.cpp Cleanup

Remove `bool prev_trigger_left_` from `AppState` and the edge-detection block
(lines ~326–335). **Defer this until `node_speech_to_text` is implemented and
the full PTT graph chain works.** For this work item: implement and register the
node only.

## Registration

```cpp
#include "trigger_edge.hpp"
state.registry_.register_builtin(make_descriptor<TriggerEdge>());
```

## Test

`trigger_edge.test.cpp`:
- press fires exactly once on rising edge, not on sustained hold
- release fires exactly once on falling edge
- held is 1.0 while above threshold, 0.0 otherwise
- threshold slider changes the detection point

## Acceptance Criteria

- Node compiles, registers, and passes unit tests
- `sh/build.sh` passes
