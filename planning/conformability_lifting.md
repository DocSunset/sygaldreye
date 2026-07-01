# Conformability lifting — implementation plan (arc started 2026-06-12)

Design: kanban/backlog/conformability.md (ratified). This file tracks
the build.

## Reality check on "the executor lifts"
The executor sees opaque descriptors — it cannot lift what it cannot
see inside. Lifting therefore lives in the DESCRIPTOR MACHINERY: node
authors write a per-sample kernel + an endpoints struct; a reusable
helper stamps the lifting loop (channel map × frame scan) at compile
time. pfr needs real named fields, so endpoint structs stay authored —
what disappears is the hand-written buffer/channel/dt plumbing.

## Rungs
1. **lift_audio helper** (ugens/synth_core): given kernel type K with
   `float tick(float)` (+ optional per-channel state), stamp:
   - per-channel instance vector (map over channel axis, grows/wraps
     like ugen_detail today)
   - frame scan loop (time axis), planar-agnostic via at()
   Proof: BiquadNode rewritten v6 + lifted; ugens tests must pass
   unchanged (same semantics).
2. Convert remaining processor ugens (vca? trivial; delay, shaper,
   sample_hold, adsr) — v6 endpoints + lift_audio. Generators (osc,
   noise, grain) keep Gen-paced loops (time-source nodes).
3. Scalar-over-span lifting (forest route 1: lifted generators) — needs
   node-instancing; design with the editor's graph_source/each work.
4. Named-shape Span header + planar audio + interleave at engine edge.
5. True GPU instancing in mesh_instances.

## Status
- [x] rung 1: Lift<K> stamp (ugen_detail) + BiquadNode v6+lifted; ugens tests green. CONVENTION: processor in=audio, out=audio_out (matches combiners).
- [x] rung 2: vca, delay, shaper, sample_hold, adsr all v6+lifted; rename cascade scripted through all presets + scene jsons; device replay verified (bell/rain/dac levels healthy through fully-lifted chains).
- [x] rung 3 (Part I, lifting_and_editor_decomposition.md):
  - L0: PortInfo::cell_rank() + whole derive from kind; whole<T> opt-out
    (V6Whole, schema "whole":1); ABI v7 lift_kind/lift_key from optional
    static node members. No behavior change.
  - L1: build_plan detects excess-rank edges (span producer → cell-kind
    consumer, not whole) → LiftGroup; tick_graph replays via Clones (N
    desc->create() instances, builtins AND subgraphs); outputs gather into
    a plan-owned Span the consumer points at. State survives reorder
    (keyed by lift_key cell value, else index), resize, and migrate_graph
    (Graph::lifted_store). connection_legal permits span→cell + cell→span.
  - L2: CellMap (stateless) loop; resource-holder guard (hard error w/
    message); subgraph infers resource_holder from inner nodes. Marked:
    dac, spawner, editor, udp send/recv, mic, sample/wav, tts/tts_local/
    stt_whisper/whisper/speech_to_text, rd_gpu, render_target/texture_view.
  - L3 "every port ready" AUDIT (cells fall out of kind — see L0):
    * whole-by-kind (cell_rank 0, never lifts) needs NO opt-out: every
      audio port (ugens mix/mc_pack/vca/biquad/…, spectrogram FFT, dac,
      spatialize), every span port (color_mesh/sprite/wire_mesh positions
      = the instanced-draw boundary instances, rung 5), every mesh/
      surface/drawable/texture/draw_call/text port.
    * cell-kind ports (scalar/vec2/vec3/vec4/quat/mat4) across math_nodes,
      osc/ptt_gate/trigger_edge, sun_light/fly_camera/hand/xr_sources,
      mesh deformers, cube/terrain/sky/water/wire/lissajous/chladni/rd/
      aurora/star_field/particle/text_label, ui/poke/editor: lift by kind
      (a vec3 input fed an N×3 Span → N instances). This is the INTENDED
      behavior, not an opt-out — confirmed no-op.
    * whole<T> opt-out applied: NONE needed today. No existing node has a
      cell-kind input that consumes an N×cell array as a unit; "whole"
      intent falls out for free via audio/span kind, exactly as designed.
      whole<T> awaits future reduce/gather/flocking nodes.
    * lift_key: meaningful on lifted generators/subgraphs whose identity
      is data (tree_generator seed — L4; editor card id — Part II). No
      builtin processor needs it (audio channels lift by index).
