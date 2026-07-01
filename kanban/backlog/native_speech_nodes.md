# Native speech nodes + text edges (ratified direction 2026-06-12)

## stt_whisper node — Quest + browser (linux comes free)
- whisper.cpp (ggml; nixpkgs for host dev, NDK cross-compile for Quest,
  official WASM path for browser later in this item).
- Ports: audio in (mic edge) + gate/trigger (PTT buttons wire straight
  in) → transcript out as a TEXT EDGE (see below). Inference on the
  worker region, 2-3 threads (Quest thermal/render budget).
- Quest payoff: on-device transcription, no PC hop — headset voice works
  with the laptop off. Model: ggml base.en (~142MB) pushed like presets
  (sh/push_graphs.sh pattern); peer-without-weights simply doesn't
  register the node (capability = advertisement; bridge covers the rest).
- Browser rung (after quest): needs -pthread web build + COOP/COEP
  headers + model fetch into MEMFS — separate verifiable step.

## tts_melo node — linux only (the only TTS consumer is claude_tmux's
## Stop hook, which is linux-bound)
- MeloTTS.cpp (OpenVINO) — best perf on the Intel host; sherpa-onnx is
  the fallback if OpenVINO packaging fights nix.
- Ports: message text-in + audio OUT EDGE: the voice is a block-region
  source — spatialize it (Claude speaks from a place in the room),
  delay it, watch it on the spectrogram. Model stays loaded in-node →
  kanban/tts_warm_process dies properly. Synthesis on the worker;
  replaces tts_cli.py / the python venvs in the speaking path.
- speak_last.sh keeps POSTing /param {"node":"speak",...} — node swap is
  invisible to the hook.

## Text endpoint type (executor push — do FIRST, both nodes want it)
- PortValue gains std::string payload; "text" becomes a wirable kind,
  EVENT rate in port_types (rate_of("text") → Event).
- ABI: set_text_in + emit_text (v6 fields, appended — old plugins keep
  loading via the version check); apply_value/output_ctx/region
  crossings inherit via the shared dispatch (queue mapping carries
  strings across the block boundary like bangs — counted, never
  dropped... strings need a real queue not a counter: EvQueue grows a
  payload deque).
- Retires the seq-bump pattern (executor step 3's last remainder):
  claude_tmux.message becomes a text-event inlet; whisper transcript →
  claude.message is AN EDGE; companion-era /param+seq stays working as
  the param path during transition.
- Editor: text edges get a wire color; text inlets stay
  persistence-only widgets until a VR keyboard exists.

Order: text edges → tts_melo (linux, replaces python voice path,
end-to-end voice loop all-native) → stt_whisper quest → stt_whisper
browser.

## Text edges LANDED (2026-06-13 overnight)
PortValue carries std::string; ABI v6 emit_text/set_text_in; stt node
now OUTPUTS the transcript (text + heard bang) instead of dropping it;
tts gained a say bang (message = cold inlet, Pd-style). seq inputs
deprecated but working (companion.py still posts them). REMAINING for
the full graph-wired voice loop: the net bridge must mirror text values
across peers (stt.text on device → claude.message on host) — fold into
the whisper-on-quest work.

## Progress 2026-06-12 (day)
- stt_whisper LANDED: whisper.cpp vendored as flake source, built by
  host + NDK from one version; node with the PTT contract, transcript
  as text edge (text + heard bang), warm context, worker inference.
  HOST GATE PASSES (espeak phrase → correct transcript, base.en).
- tts_local LANDED (sherpa-onnx C API per the ratified fallback; runs
  piper AND melo-family vits voices; default en_US-ryan = MALE): warm
  in-process synthesis, message cold inlet + say bang, voice leaves as
  an AUDIO EDGE (block source — spatializable). THE ROUND-TRIP GATE
  PASSES: tts_local speaks, stt_whisper transcribes it correctly — the
  machine verifies its own voice loop.
- Device: whisper runs ON-QUEST (model via adb push to the external
  files dir — 25 MB/s; /models HTTP route added but >3 MB POSTs still
  die even with MG_MAX_RECV_SIZE raised — investigate mongoose growth
  path); inference threads spin, text reaches /values.
- BLOCKED on physics: Quest delivers ZERO mic frames while the headset
  is un-worn (permission granted + session running + both callback and
  pull modes silent; prox_close broadcast didn't unmute). Needs a worn
  headset for the live loopback — or a setting that unmutes when
  doffed. Whisper's silence-hallucination (" you") confirmed the takes
  were empty, not garbled.
- Browser/wasm rung: NOT STARTED (build/web exists; needs -pthread +
  COOP/COEP + MEMFS model fetch).

## Deferred (2026-06-12, Travis)
- Browser/wasm whisper rung (build/web needs -pthread + COOP/COEP + MEMFS model fetch)
- On-device whisper speedup timing verification (needs worn headset mic)
- On-device TTS latency timing
