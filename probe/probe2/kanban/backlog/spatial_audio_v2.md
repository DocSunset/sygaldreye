# Spatializer v2: third-party HRTF (headphone session verdict 2026-06-12)

Parametric HRTF judged by ear: stereo panning, no externalization,
behind-you reads as inside-head and louder, elevation imperceptible.
Rolling our own further = bad ROI.

DECISION (Travis veto pending): Steam Audio — open source Apache,
measured-HRTF convolution, official Android/ARM64 + Linux, C API; slots
behind the UNCHANGED spatialize node ports; brings occlusion/reflections
later. Alternatives logged: Resonance Audio (lighter, stagnant), Meta
Audio SDK (best Quest integration, platform-locked — wrong for a node
that runs on linux/browser peers too). hrtf_params subgraph form RETIRES.
Keep per-ear level outputs (agent evidence) and the distance/listener
math; only the binaural core swaps.

## Device findings 2026-06-12

- spatialize goes PERMANENTLY silent when source pos == listener pos
  (both default (0,0,0)): probed 1.5e-44 denormals out. Distance-zero
  guard (min distance clamp ~0.1 m) needed regardless of which HRTF
  library sits underneath. Live workaround: always wire/offset pos.
- With pos wired (latch or const), spatialize + bus + dac all probe
  healthy on device, including the full playground chain. The
  "never heard bell/rain on headset" report could not be reproduced
  once presets were correctly registered — suspect stale/garbage
  preset registrations from the /plugins name-sniffing bug.
