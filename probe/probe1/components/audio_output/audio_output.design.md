# audio_output

Platform audio I/O. Delivers stereo interleaved float buffers to a caller-supplied callback at a configurable sample rate (default 48 kHz).

## Ports

**Inputs**
- `AudioCallback cb` — synthesis callback supplied at construction; called per block

**Outputs**
- Android: PCM audio via AAudio to the device DAC
- Host: `audio_output_test.wav` written on `stop()`

**Temporal couplings**
- `start()` must be called after `create()`; `stop()` terminates the stream/loop

## Requirements

- Stereo interleaved float (`left0, right0, left1, ...`)
- Low-latency mode on Android (~256 frame buffer)
- Host implementation drives callback synchronously; writes f32 WAV on completion
- Callback must be real-time safe (no allocation, no locks, no syscalls)
- Move-only; not copyable

## Allowed dependencies

- Android: `aaudio` (NDK)
- Host: C++ stdlib only

## Owning package

`audio`
