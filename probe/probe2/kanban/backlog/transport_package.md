# Transport package — shared musical clock over the mesh

Sketched 2026-07-04 (architecture session). NOT language design — a capability
package spending only ratified vocabulary (ADR-015/020/021/022). File under
"the gap dissolved into a package."

- **Vocabulary**: transport node (publishes the beat clock; tempo value in;
  start/stop/seek event ports), quantize mapping (gates events on beat
  boundaries by evaluating the beat function), beat-clock kind.
- **Beat is a time-parametric value** (ADR-022 kind): beat = f(local
  monotonic time) as rate+offset coefficients — consumers evaluate at their
  own clock, sample-accurately, no tick traffic.
- **Machinery**: Link-shaped convergence (peerless tempo/phase averaging over
  the mesh's authenticated links; joiners snap to session; ~ms alignment).
  Licensing check if adopting libableton-link (GPLv2+) vs implementing the
  protocol shape ourselves. Pairwise clock offsets ± uncertainty published as
  demand-driven data (QoS pattern); cross-peer event timestamps translate
  through them.
- **Prior art to study (Travis, 2026-07-04)**: NTP's clock *discipline* — not
  just offset estimation (symmetric exchanges, filtering/selection) but the
  PLL/FLL loop that STEERS the local estimate smoothly: slew, never step
  (a stepped clock is an audio click). PTP (IEEE 1588) — BMCA master
  election + hardware timestamping at the PHY for sub-µs on wired LAN; shows
  both the ceiling and its cost (consumer wifi/Quest lack PHY timestamps, so
  PTP-grade sync is a wired-peers tier, not a mesh baseline). Realistic
  tiers: NTP-discipline filtering over mesh links (ms, everywhere) →
  PTP-assisted where wired (µs, e.g. Linux↔Linux). Link's own offset
  estimator is the same family; compare all three before building.
- **Honest limit**: aligned, not sample-locked — software cannot word-clock
  two dacs; drift is absorbed by a visible resampler mapping at the net
  crossing, ratio fed by measured offset drift. Sample-lock = shared hardware
  clock, out of scope.
- Capture testimony wall-clock stays descriptive; beat clock is performance
  time and never enters identity or ordering.
