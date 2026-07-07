# audio_region follow-ups

- nix rpath CMake warning on audio_region_test (SDL2 from /nix/store vs
  /usr/lib): benign, tests pass via LD_LIBRARY_PATH; silence properly.
- RT purity: block scheduler allocates (store_ keys, buffers) — fine at
  256-frame blocks on desktop; tighten before low-latency device use.
- Browser autoplay: SDL resumes Web Audio on first user gesture; add a
  visible "tap to enable sound" affordance in the web shell.
- Device synths (atmos/rain/…) still own private AAudio streams — migrate
  them to audio edges + the shared dac during a headset session.
- dac assumes mono in → stereo dup; multichannel + a mixer node later.

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
