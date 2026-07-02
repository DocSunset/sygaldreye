# audio_region follow-ups

- nix rpath CMake warning on audio_region_test (SDL2 from /nix/store vs
  /usr/lib): benign, tests pass via LD_LIBRARY_PATH; silence properly.
- RT purity: allocation is still reachable inside render_block — fine at
  256-frame blocks on desktop; tighten before low-latency device use.
  Precise residual (2026-07-01, post Lift-resize fix): block-scheduler
  store_ keys; Lift kernel construction + DelayLine::prepare first-touch
  line buffer when a live edit GROWS a channel count (shrink also frees
  stateful kernels' inner buffers); Lift::buf / Mix / McPack / dac buffer
  growth past capacity on first sight of a larger block. All removed by
  the warm pass below.
- Sample rate: processors now honor in.sample_rate, but generators (Gen,
  metro) and audio_region's kRate stay pinned to 48000 — a 44.1k device
  still detunes at the source until the engine plumbs the device rate
  into the block region.
- Browser autoplay: SDL resumes Web Audio on first user gesture; add a
  visible "tap to enable sound" affordance in the web shell.
- Device synths (atmos/rain/…) still own private AAudio streams — migrate
  them to audio edges + the shared dac during a headset session.
- dac assumes mono in → stereo dup; multichannel + a mixer node later.
- Warm pass after graph swap (audit 2026-07-01): the remediation arc
  preserves surviving channels' kernel state on channel-count change,
  but growth still allocates in the callback (kernel construction +
  first-touch prepare, e.g. DelayNode's per-channel buffers → xrun
  risk). Design an off-thread warm pass: after a swap, prepare every
  block node for its wired channel count before the callback sees the
  new plan. reports/audit_conformability_editor_arc.md §6.

## Epoch-poisoned absolute-time state (FIXED for metro, 2026-06-12 overnight)

The night's marquee bug: graphs containing chime_synth_g went silent
nondeterministically — only after certain swap histories, never on a
fresh boot, never on host. Root cause: MetroNode::next_ stores ABSOLUTE
time in whatever epoch last ticked it. Frame region runs on XR time
(~1e5 s); the block scheduler on stream time (~minutes). Any swap
history that ticks a metro in frame region (e.g., a chime not yet wired
to a dac) then migrates it into block region leaves next_ ~1e5 seconds
in the future: the metro never fires again, and migration faithfully
carries the bricked state through every subsequent graph. Rain (unclocked
grain_cloud) was immune, which is what made the bell/rain split so
confusing. Confirmed with a deterministic 2-graph repro on device;
fixed by resyncing when next_ > t + period.

DESIGN LESSON (feeds kernel extraction, overnight 3): region migration
changes a node's TIME BASE. No node should keep absolute-time state;
kernels must receive dt (Gen::frames already does this and was immune).
Make epoch-robustness a stated kernel-contract invariant.

Also landed: GET /plan (executor observability: order/block_order/
appliers/delays/crossings as JSON) — first-class agent eyes into the
scheduler; diagnosing this bug without it would have needed device gdb.
