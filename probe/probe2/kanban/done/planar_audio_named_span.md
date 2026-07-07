# Planar audio + named-shape Span header (conformability rung 4)

Task #58. Span today is an anonymous rank-≤2 float view (`data, rows,
cols`, frame-leading). To lift correctly the executor must tell a
2-channel buffer from a 2-frame buffer, and audio must go planar
(channel-major) internally for SIMD-friendly per-channel scanning.

## Two coupled changes

### 1. Named-shape header on Span
Generalize AudioBuffer's implicit (channels × frames) into an explicit
small named-axis header on Span (conformability.md §"Audio as named-axis
matrix"): axes carry IDENTITY (channel, time, …), not just counts. This
is what disambiguates "256 channels vs 256 samples" — by axis name, not
by which dimension is bigger.

- Keep it POD + a tiny header (rank ≤ 2 for now). NOT owning Eigen (no
  allocation on the audio path; heavy dep for the freezer's embedded
  targets), NOT vector<vector>. Eigen::Map stays an opt-in zero-copy
  ADAPTER only.
- Kernel signatures must not preclude rank > 2 / tensor payloads later
  (designed-for, not built).

### 2. Planar (channel-major) audio internally
Go planar inside the graph so lifted channel instances scan contiguous
memory. **Interleave only at the engine/device boundary** (audio_output
/ DAC). Audit every audio producer/consumer for interleave assumptions;
the boundary conversion is the single allowed interleave point.

## Dependencies / sequencing

- **Blocks #57** in practice (the Lift shell can't tell scan from map
  without named axes) and **#59** (general lifting reads the same
  header). This is the foundational rung — do it before broad kernel
  migration.
- Touches: Span (sygaldry_endpoints), AudioBuffer call sites,
  audio_region, audio_output interleave boundary, spatialize (the
  axis-consuming/producing node — its semantics only express once axes
  are named).

## Acceptance

- Span carries named axes; /values still renders a sensible "span[NxM]"
  (now axis-labelled).
- Audio path is planar end-to-end with one interleave at the device
  boundary; existing synths sound identical (ear A/B on device).
- A channel-axis vs time-axis Span are distinguishable at the type/
  header level (unit test), not by heuristic.

## Completion (2026-06-14, task #58)

Done and host-verified (all suites green). Headset ear A/B still pending.

**Named-shape header.** `Axis{Item,Cell,Channel,Time}` + `axis_name()` in
sygaldry_endpoints. `Span` gained `row_axis`/`col_axis` (default Item×Cell,
so every existing N×M producer is unchanged). Axes survive the C ABI:
`emit_span` carries two extra ints; threaded through tick, subgraph, and
`/values` now prints `span[N item x M cell]` (or channel/time). Unit tests
`SpanAxesCarryIdentityNotHeuristic` + `SpanDefaultsToItemCell` prove a
2-channel and a 2-frame span are told apart by NAME, not by count.
AudioBuffer stays the typed audio view (channels×frames implicit-named); it
unifies into Span at the executor rung (#59).

**Planar audio.** Internal layout flipped to channel-major in
`ugen_detail::at` + `Lift` (channel-outer scan, contiguous), `mix`,
`mc_pack`. SampleHold's shared cross-channel clock became a per-frame
time-axis mask (order-independent — the planar scan must not reach across
channels per frame). spatialize/hrtf now emit planar stereo. THE single
interleave point is the dac→device de-planarize in
`audio_region::render_block`. Audited every AudioBuffer site: remaining
interleave is legitimate (device boundary, on-disk WAV, or mono). Ring
crossings stay flat-FIFO — valid only for mono block→frame audio (all
current scope/STT/spectrogram consumers); a multichannel ring would need
boundary interleave like the device (deferred).
