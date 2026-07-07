# Node: `mic_input`

## Goal

Wrap the existing `MicCapture` component as a graph node that outputs an `AudioBuffer`
port each tick, containing the microphone samples captured since the previous tick.
This replaces the hardcoded `state.mic_` setup in `app.cpp` (lines 200–208) and allows
downstream nodes (ptt_gate, speech_to_text) to receive microphone audio through the
graph.

## Feasibility Assessment

**Feasible with careful threading.**

`MicCapture` invokes its callback on an audio thread at ~16 kHz. The graph tick runs
on the render thread (~72 Hz). These two threads must share a sample buffer without
data races.

**Design:** use a mutex-guarded `std::vector<float>` ring accumulator in the node:
- Audio thread callback: `lock(mutex_); accum_.insert(end, samples, samples+frames); unlock()`
- `operator()` (render thread): `lock(mutex_); swap(accum_, tick_buf_); unlock();` then
  sets `outputs.audio_out.value = AudioBuffer{tick_buf_.data(), frames, 1, 16000}`

`AudioBuffer` holds a borrowed pointer. The tick_buf_ vector member lives for the
lifetime of the node and is valid for the entire tick (process → graph propagation →
downstream node consume). This is safe because the graph is ticked serially on the
render thread.

`MicCapture::create` returns `std::optional<MicCapture>`. The node must handle
creation failure gracefully: if `mic_` is nullopt, output an empty AudioBuffer
(data=nullptr, frames=0).

`MicCapture` is not copyable; `MicInputNode` must be move-only or hold it via
`std::optional<MicCapture>`. Since graph nodes are created via `new` and held as
`void*`, move semantics don't matter — just store `std::optional<MicCapture> mic_`.

**Lifecycle:** `MicCapture::create()` requires the audio subsystem to be available,
which it is after `android_main` starts. Create it in the node's `create()` callback
(which runs on the render thread during graph parse, inside the frame loop). The
`start()` call must also happen there. `stop()` happens in the destructor.

## Component

`components/mic_input/`

```
mic_input/
  mic_input.hpp
  mic_input.cpp
  CMakeLists.txt
```

## Design

```cpp
class MicInputNode {
public:
    static consteval std::string_view name() { return "mic_input"; }

    struct inputs {
        slider<"gain", "", float, fp(0.f), fp(10.f), fp(1.f)> gain;
    } inputs;

    struct outputs {
        port<"audio_out", AudioBuffer> audio_out;
    } outputs;

    MicInputNode();
    ~MicInputNode();

    void operator()(double time_s);

private:
    std::optional<MicCapture> mic_;
    std::mutex                mutex_;
    std::vector<float>        accum_;   // written by audio thread
    std::vector<float>        tick_buf_;// swapped in on render thread each tick
};
```

Constructor: calls `MicCapture::create(callback)` and if successful calls `mic_->start()`.
The callback: `[this](const float* s, int n){ lock; accum_.insert(end,s,s+n); unlock; }`

`operator()`:
1. `{ lock; swap(accum_, tick_buf_); }` — O(1) pointer swap, minimal lock hold time
2. Apply gain: multiply tick_buf_ samples by `inputs.gain.value`
3. `outputs.audio_out.value = {tick_buf_.data(), (int)tick_buf_.size(), 1, 16000}`
   (or empty buffer if tick_buf_ is empty or mic_ is nullopt)

## App.cpp Cleanup

Remove from `AppState`:
- `std::optional<MicCapture> mic_`
- `PushToTalk push_to_talk_`

Remove from `android_main`:
- `state.mic_ = MicCapture::create(...)` and `state.mic_->start()`
- `state.push_to_talk_.set_companion_url(...)`
- The trigger-based PTT logic in the frame loop (lines ~326–335)
  **only after** `node_trigger_edge`, `node_ptt_gate`, and `node_speech_to_text` are done

For this work item: only implement the node. Do not yet remove app.cpp PTT logic —
that cleanup is deferred to `node_speech_to_text.md` which is the last PTT node.

## Registration

```cpp
#include "mic_input.hpp"
state.registry_.register_builtin(make_descriptor<MicInputNode>());
```

## Test

`mic_input.test.cpp` — host-side test (mock the audio callback):
- Construct node; inject samples via callback; call process(); verify AudioBuffer
  has correct frame count and data pointer is non-null
- Verify gain scaling: inject 1.0 samples with gain=2.0 → output ≈ 2.0

## Acceptance Criteria

- `mic_input` node compiles and registers
- Audio flows to `audio_out` port each tick when mic is available
- Empty AudioBuffer when mic creation fails
- Unit test passes on host
- `sh/build.sh` passes
