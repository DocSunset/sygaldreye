#pragma once
#include <functional>
#include <optional>

// Called from the audio thread (Android) or synthesis loop (host).
// Must fill `frames` stereo float samples into `out` (left0,right0,left1,...).
// Must be real-time safe: no allocation, no locks, no syscalls.
using AudioCallback = std::function<void(float* out, int frames)>;

class AudioOutput {
public:
    struct Impl;
    static std::optional<AudioOutput> create(AudioCallback, int sample_rate = 48000);
    void start();
    void stop();
    ~AudioOutput();
    AudioOutput(AudioOutput&&) noexcept;
    AudioOutput& operator=(AudioOutput&&) noexcept;
    AudioOutput(AudioOutput const&) = delete;
    AudioOutput& operator=(AudioOutput const&) = delete;
private:
    Impl* impl_ = nullptr;
    explicit AudioOutput(Impl*);
};
