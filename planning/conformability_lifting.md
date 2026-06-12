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
- [ ] rung 2: delay, shaper, sample_hold, vca, adsr (NOTE: vca out-rename cascades into ALL presets + playground jsons — script it)
