# spectrogram

Rolling STFT spectrogram node: the agent's ears (vision.md: agents can't
hear; audio inspection emits images). Audio edge in → CPU ring image
maintained. The former GL texture output was removed with the render_region
boundary (GL lives only there); its viewer (texture_view) is parked. The
visual output returns via the render_payloads ImageTex path on the offscreen
leg (kanban/backlog/offscreen_fbo_leg.md) — `image()` is the ring to expose.

## Ports

- Inputs: `audio` (AudioBuffer, any rate/channel count — `channel` selects);
  `gain` (display gain into the log scale); `columns` (ring width =
  visible history; reallocates on change).
- Outputs: — (the ring is `image()`, width × 256 bytes, column-major;
  x = time (wraps at the cursor), y = frequency, DC at the bottom).
- Sources: —
- Destinations: the offscreen-leg ImageTex consumer, when it lands.
- Temporal couplings: audio arriving faster than ticks accumulates and
  drains kHop at a time.
- Intended seams: FFT size/hop are compile-time (kFft 512 / kHop 256) —
  variants become sibling nodes rather than runtime knobs.

## Requirements

- No GL, no allocation outside `columns` changes; Hann window, radix-2 FFT
  (no dependencies), log-magnitude display.

## Allowed dependencies

sygaldry_endpoints

## Owning package

scene

## Future

When the audio region lands, audio(block) → image(render) crossing makes
this node's input a ring mapping boundary — the node itself stays
frame-region.
