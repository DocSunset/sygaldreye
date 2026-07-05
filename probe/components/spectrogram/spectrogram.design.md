# spectrogram

Rolling STFT spectrogram node: the agent's ears (vision.md: agents can't
hear; audio inspection emits images). Audio edge in → ring texture out;
view through texture_view, capture through /screenshot.

## Ports

- Inputs: `audio` (AudioBuffer, any rate/channel count — channel 0 used);
  `gain` (display gain into the log scale); `columns` (ring width =
  visible history; reallocates on change).
- Outputs: `texture` (R8, columns × 256; x = time (wraps at the cursor),
  y = frequency, DC at the bottom, Nyquist at the top).
- Temporal couplings: ticks on the render thread (owns a GL texture);
  audio arriving faster than ticks accumulates and drains kHop at a time.
- Intended seams: FFT size/hop are compile-time (kFft 512 / kHop 256) —
  variants become sibling nodes rather than runtime knobs.

## Requirements

- Constructor GL-free; texture created lazily on first tick.
- Hann window, radix-2 FFT (no dependencies), log-magnitude display.

## Allowed dependencies

sygaldry_endpoints, gpu_texture, GLES3.

## Owning package

scene

## Future

When the audio region lands, audio(block) → texture(render) crossing
makes this node's input a ring mapping boundary — the node itself stays
frame-region (the type system forbids audio+GL on one node only for
BLOCK-rate audio ports; its input is the ring's frame-side view).
