# Device block-region intermittent silence (OPEN; analysis corrected 06-13)

## What is REAL (device, verified with /graph + key-presence checks)
- w_full (35 nodes) active and confirmed, frame side ticking (latch
  sources move), /plan healthy and identical to alive runs — yet ALL
  block-side snapshots frozen at exact 0 (sp_bell.level_l,
  sp_rain.level_l, out.level). Intermittent across swap histories;
  restart + replay cures; posting all-fresh-id graphs cured it at least
  once (needs re-verification, see contamination note).

## What was a MIRAGE (lessons, expensive ones)
- The "host repro" never happened: w_full needs 7 device-only types
  (head_pose, controllers, renderer, mic_input, wav_player,
  speech_to_text) → parse fails on host → POST /graph RETURNED the
  error but the hunt scripts piped responses to /dev/null → probes then
  read MISSING keys via v.get(k, 0) and printed "dead". Every host
  alive/dead flip-flop was the default graph answering.
- Some device flip-flop data may share the artifact (responses not
  echoed in loops). The two PROTOCOL RULES for all future probing:
  (1) always echo the POST response; (2) always distinguish missing
  keys from zero values.

## Leading hypothesis: zombie AAudio stream
We never install an AAudio error/disconnect callback. A disconnected
stream (route change, device idle, repeated open/close) stops invoking
the data callback FOREVER; stream_ stays non-null so pump_offline
declines; block region fully stalls; snapshots freeze at 0; frame side
unaffected. Fits everything except (unverified) the fresh-id cure —
rebuild_unlocked does NOT recreate an existing stream, so if fresh ids
genuinely cured a dead state, there is a second mechanism.

## Next session protocol
1. Add AAudioStreamBuilder_setErrorCallback → log + flag; on
   AAUDIO_ERROR_DISCONNECTED recreate the stream (the documented
   pattern). Expose stream state in GET /plan or a /audio route.
2. Re-collect device evidence with the two protocol rules, including
   whether fresh-id graphs really cure (if yes: second mechanism).
3. Instrumentation kept (env-gated SYGALDREYE_BLOCK_DEBUG in
   audio_region.cpp: [blk-pre]/[blk]/[reb]/[pump] traces).

## Related real fixes already landed en route
- metro epoch resync (audio_region_followups.md) — one real mechanism
  of history-dependent silence, deterministically repro'd + fixed.
- hot-reload re-parse vs /graph ordering race (ugen_followups.md) —
  still open, entangled in kill sequences.

## SOLVED (mechanism found 2026-06-13 ~07:00): THE MIC

Honest-probe bisection on device: w_full WITH mic_input → block output
silent (snapshots frozen 0); w_full WITHOUT mic → alive (0.042/0.019/
0.029); t_bell alive on the same process while w_full dead (swap had
destroyed the mic → output recovered). The mic's capture stream
(PERFORMANCE_MODE_NONE input) conflicts with the LOW_LATENCY output
stream on Quest: output stalls (silence) or degrades (the "horribly
distorted" dings). Explains the ORIGINAL report perfectly: the
playground always carried the mic, so Travis NEVER heard chime/rain.
Intermittent alive runs = the conflict is racy, not deterministic.
Zombie-stream hypothesis refuted (t_bell played while w_full dead);
error callback + recovery landed anyway (3986cde, harmless + correct).
Metro epoch fix remains real and necessary (separate mechanism).

FIX (next session, with Travis live for the voice loop): single audio
thread — input stream WITHOUT its own callback; the OUTPUT callback
pulls mic samples via non-blocking AAudioStream_read. One callback,
one lifecycle (also subsumes mic teardown crash, mic_garbage_samples).
Until then the shipped playground omits the mic node.

## ACTUALLY SOLVED (2026-06-12 morning, with Travis's audio-engine design)

THE root cause, found by drilling [blk]→[sp]→[hrtf] probes to the exact
dead value: **uninitialized Eigen port values**. Legacy port<vec3> did
`T value{}` and Eigen's default ctor leaves GARBAGE — an unwired
spatialize.listener_pos read whatever the heap held. One boot sp_bell
drew inf → rel = pos - inf → distance ∞ → dist_gain 0 → exact zeros.
Per-boot/per-instance/per-allocation dice explain EVERY mystery of the
last two days: nondeterministic silence, fresh-ids "curing" it (new
allocations), host/device divergence, moving-vs-static red herrings.
The mic correlation was real but secondary (stream churn/conflicts) and
fell to the AudioEngine single-owner rework.

Fixes landed:
- port<T> defaults through endpoint_default (vec zero, quat/mat
  identity) — kills the entire class at the type level. (Exactly the
  footgun the endpoints-v6 spec predicted in writing.)
- AudioEngine singleton (ratified Pd model): ONE owner for audio
  hardware; output stream persists across all graphs; all dacs sum;
  mic is a shared-ring tap; wav_player taps engine.play(); /play is
  graph-independent.
- XR-optional launch: peer fully alive (HTTP/graph/audio) before/without
  a session; offline_tick while doffed. `setprop debug.oculus.guardian_pause 1`
  bypasses the boundary dialog that blocks controller-less launches.

Verified: mic playground alive 3/3 fresh boots (bell+rain+dac), /play
mixes without touching the graph, 28 signal_graph tests green.
