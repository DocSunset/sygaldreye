# Kernel extraction: every DSP node = per-sample kernel + generated Lift shell

> DONE 2026-06-14 (149c832 + synth_core dep commit). On opening the code,
> most of this was already landed in the overnight arc: every DSP
> primitive is a kernel and the PROCESSOR half uses `ugen_detail::Lift`.
> The remaining delta — the generator half — is now done:
> - `synth::Gen` (synth_core/block_shell.hpp): the generator-side stamp,
>   symmetric to `Lift`. noise/perc/grain_cloud lost their hand loops;
>   osc now uses `synth::Phasor` + `synth::Gen` (fixes a latent
>   saw/square/tri block-end wrap; sine unchanged).
> - Exceptions documented in-source: metro (time/event), spatialize
>   (axis-consuming HRTF), dac (resource holder, never lifted).
> - Guard: ugens.test Generators.ShellsProduceSoundThroughGenerate;
>   32/32 host + android clean. Details in planning/kernel_extraction.md.
> DEFERRED to #58/#59 (correctly, by design — see below): named-axis
> scan-vs-map disambiguation (#58) and lifting generators over param
> arrays (#59). Device ear-A/B optional (changes are RMS-guarded /
> behavior-preserving).

Task #57. The ratified form (conformability.md §"Audio as named-axis
matrix"): **per-sample kernels are the ONLY authored form; block
processing is the executor's lifting loop, stamped per kernel type at
compile time** (make_descriptor-style template machinery) so generated
shells match today's hand-written block performance. Hand-maintained
block ugens disappear.

## Where it stands

- Audio block path works (audio_region renders blocks; ugens carry
  hand-written block loops today).
- Span payload + ABI emit_span landed; first manual lift (scatter →
  mesh_instances) proven visually.
- NOT done: the compile-time Lift shell that wraps a scalar kernel into
  a block/array processor. Every ugen is still hand-written block code.

## Scope of this item

1. Define the **kernel concept**: a node exposing a per-sample (per-cell)
   `step()` over its natural-rank cell + declared state, separate from
   any block loop. Pick the authoring shape (free function? struct with
   `step`?) — must be `make_descriptor`-friendly and carry declared
   ranks (conformability.md needs axis identity: channels = map, time =
   scan).
2. Build the **Lift template**: given a kernel, generate the block shell
   (time axis = scan, threading state sample→sample; channel axis = map,
   N independent kernel instances with per-channel state). This is the
   audio special case generalized — match channels vs samples by AXIS
   IDENTITY, not count.
3. **Migrate the ugens** kernel-by-kernel, A/B each against the current
   hand-written block output (bit-exact or ear-verified). Order: start
   with the simplest stateless (shaper, gain) then stateful scan (osc
   phase, biquad, slew, delay).
4. **Documented exceptions** (do NOT force into the kernel mold):
   - `metro` / `perc` trigger nodes — event-rate, not sample-rate.
   - `spatialize` — CONSUMES the channel axis, produces a new one
     (axis-producing; needs the named-axis machinery from #58 first).
   - resource-holders (dac) — never lifted (conformability.md).

## Dependencies / sequencing

- The named-axis Span header (#58, planar audio) is what lets the Lift
  shell tell channels from samples. **Do #58 first** (or in lockstep):
  the scan-vs-map disambiguation is unimplementable on an anonymous
  rank-2 Span.
- Enables #59 (general lifting): once kernels are the authored form,
  lifting a scalar kernel over a Span is the same template at the
  graph level.
- Freezer territory (do NOT build now): derived block-optimized /
  SIMD kernels generated from the authored one.

## Acceptance

- One representative stateful ugen (e.g. biquad) authored as a pure
  per-sample kernel, its block shell generated, output matching the
  current hand-written version on the host audio path.
- metro/spatialize explicitly documented as non-kernel in their design
  docs with the reason.
