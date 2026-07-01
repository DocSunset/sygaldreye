# audio_output component

Platform audio I/O: an AAudio callback on Android (Quest 3) and an offline PCM/WAV writer on the host. Delivers stereo interleaved float buffers to a caller-supplied synthesis callback at 48 kHz. The only component with platform-specific branches.

## Approach

- `audio_output.hpp` declares:
  ```cpp
  // Called from the audio thread (Android) or synthesis loop (host).
  // Must fill `frames` stereo float samples into `out` (left0,right0,left1,...).
  // Must be real-time safe: no allocation, no locks, no syscalls.
  using AudioCallback = std::function<void(float* out, int frames)>;

  class AudioOutput {
  public:
      static std::optional<AudioOutput> create(AudioCallback, int sample_rate = 48000);
      void start();
      void stop();
      ~AudioOutput();
      // move-only
  };
  ```
- **Android**: wraps AAudio (`AAudioStreamBuilder` → `AAudioStream`). Callback thread is set to `AAUDIO_PERFORMANCE_MODE_LOW_LATENCY`. Buffer size ~256 frames.
- **Host**: a thin wrapper that drives the callback synchronously in a loop, writing raw PCM to a `.wav` file (44-byte header + f32 LE samples). Used by offline render tools and tests. Pull no Nix dep for this — just POSIX file I/O.
- The two implementations live in separate `.cpp` files selected by CMake platform guard (`if(ANDROID) … else … endif()`).
- `audio_output.test.cpp` (host only): create an `AudioOutput`, write 1 second of 440 Hz sine to a WAV file, verify the file exists and has the expected byte length.

## Acceptance

- On Android: audio plays without glitches at the default buffer size
- On host: offline WAV render produces a valid file readable by standard tools
- `audio_output.design.md` present; each platform `.cpp` under 100 lines of substantive code

## Dependencies

- `synth_core` (item 01) — used in tests to generate the sine tone
