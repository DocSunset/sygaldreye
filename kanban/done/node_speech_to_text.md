# Node: `speech_to_text`

## Goal

Wrap the existing `PushToTalk` component as a graph node. Accumulates audio from
`audio_in` while a recording session is open, and POSTs the buffer to the companion
URL when a `finalize` bang arrives. This is the final piece of the PTT graph chain:

```
[mic_input] → [ptt_gate] → [speech_to_text]
[ptt_gate.opened]        → [speech_to_text.begin]
[ptt_gate.closed]        → [speech_to_text.finalize]
```

After this node is implemented and the full chain works, remove the hardcoded PTT
logic from `app.cpp` (lines 326–335, `state.prev_trigger_left_`, `state.push_to_talk_`,
`state.mic_`).

## Feasibility Assessment

**Feasible; no new infrastructure needed.**

`PushToTalk` already handles the accumulation and async HTTP POST. The node wraps it:
- On `begin` bang: call `push_to_talk_.begin_recording()`
- Each tick: call `push_to_talk_.feed(audio_in.data, audio_in.frames)` if recording
- On `finalize` bang: call `push_to_talk_.end_recording(log_callback)`

`companion_url` is a string parameter (not a port). Handle it the same way as
`text_label` — custom serialize/deserialize or a non-slider string member.

**No text output port:** strings are not in `PortValue`. The transcript is logged
to Android logcat via the same `TranscriptCallback` pattern as the hardcoded code.
A future work item can add a `std::string` to `PortValue` and expose a `transcript`
output port. Document this limitation.

**Thread safety:** `PushToTalk::feed()` is documented as "real-time safe when not
recording; push_back when recording." It is not thread-safe for concurrent calls.
Since `operator()` runs on the render thread and the audio data comes via the graph
(already on the render thread via mic_input's per-tick snapshot), all calls to
`push_to_talk_` happen on the render thread. Safe.

**Companion URL discovery:** the hardcoded `kCompanionUrl = "http://192.168.1.1:9090"`
in app.cpp becomes a serialised string param on the node with the same default. The
mDNS-based discovery that may happen later is a separate concern.

## Component

`components/speech_to_text/`

```
speech_to_text/
  speech_to_text.hpp
  speech_to_text.cpp
  CMakeLists.txt
```

## Design

```cpp
class SpeechToTextNode {
public:
    static consteval std::string_view name() { return "speech_to_text"; }

    struct inputs {
        port<"audio_in",  AudioBuffer> audio_in;
        port<"begin",     float>       begin;    // bang: start recording
        port<"finalize",  float>       finalize; // bang: POST and reset
    } inputs;

    struct outputs {
        // No ports: transcript is logged. Future: port<"transcript", std::string>
    } outputs;

    // Serialised string param
    std::string companion_url = "http://192.168.1.1:9090";

    SpeechToTextNode();
    void operator()(double time_s);

    std::string to_json() const;
    bool from_json(std::string_view json);

private:
    PushToTalk ptt_;
    bool       recording_ = false;
};
```

Constructor calls `ptt_.set_companion_url(companion_url)`. Note: if `companion_url`
changes via `from_json` (graph reload), call `ptt_.set_companion_url(companion_url)`
again in `operator()` before use, or in `from_json` directly.

`operator()`:
```cpp
ptt_.set_companion_url(companion_url);  // idempotent; cheap string assign

if (inputs.begin.value > 0.5f && !recording_) {
    ptt_.begin_recording();
    recording_ = true;
}
if (recording_ && inputs.audio_in.value.data && inputs.audio_in.value.frames > 0)
    ptt_.feed(inputs.audio_in.value.data, inputs.audio_in.value.frames);

if (inputs.finalize.value > 0.5f && recording_) {
    ptt_.end_recording([](std::string_view text) {
        __android_log_print(ANDROID_LOG_INFO, "eyeballs",
                            "transcript: %.*s", (int)text.size(), text.data());
    });
    recording_ = false;
}
```

## App.cpp Cleanup (final PTT removal)

Once this node is implemented and registered, remove from `app.cpp`:
- `state.push_to_talk_.set_companion_url(kCompanionUrl)` and the `kCompanionUrl` constant
- `state.mic_ = MicCapture::create(...)` and `state.mic_->start()` (already removed by
  `node_mic_input` if that item was done first — coordinate)
- `std::optional<MicCapture> mic_` from AppState
- `PushToTalk push_to_talk_` from AppState
- The trigger-edge detection block: `bool prev_trigger_left_`, the `begin_recording` /
  `end_recording` calls in the frame loop
- `#include "mic_capture.hpp"` and `#include "push_to_talk.hpp"` if no longer used

Update `kDefaultGraph` to include the full PTT chain:
```json
{"id":"mic",     "type":"mic_input",       "params":{}},
{"id":"edge",    "type":"trigger_edge",    "params":{}},
{"id":"gate",    "type":"ptt_gate",        "params":{}},
{"id":"stt",     "type":"speech_to_text",  "params":{"companion_url":"http://192.168.1.1:9090"}},
```
with edges connecting left_controller → trigger_edge → ptt_gate → speech_to_text.

## Custom Serialization

Implement `to_json` / `from_json` similarly to `text_label`: emit/parse a JSON object
containing both `companion_url` (string) and any slider-equivalent fields. Since there
are no sliders, just:
```json
{"companion_url": "http://192.168.1.1:9090"}
```

Register with a manually-assembled `EyeballsNodeDescriptor` (same pattern as
`text_label`), or add `HasSerialize`/`HasDeserialize` concept detection to the ABI
generator so it calls `node->to_json()` / `node->from_json()` when present.

## Registration

```cpp
#include "speech_to_text.hpp"
// manual descriptor or extended make_descriptor
state.registry_.register_builtin(make_descriptor<SpeechToTextNode>());
```

## Test

No automated test (PushToTalk sends HTTP; requires live companion). Manual acceptance:
load graph with PTT chain, hold left trigger, speak, release — verify transcript in
logcat.

## Acceptance Criteria

- Node compiles and registers
- Left trigger press → begin_recording, release → end_recording + POST
- Transcript appears in logcat
- `state.push_to_talk_` and `state.mic_` removed from AppState
- Hardcoded PTT edge-detection removed from frame loop
- `sh/build.sh` passes
