#pragma once
#include <functional>
#include <optional>

// Called from the capture stream's thread with mono float32 samples.
// Must be real-time safe.
using MicCallback = std::function<void(const float* samples, int frames)>;

// Microphone capture. Callback mode: Quest's AAudio input delivers
// reliably with its own data callback but starves a non-blocking
// pull-read (verified 2026-06-12). Ownership lives with AudioEngine —
// ONE capture stream per process; nodes tap the engine's shared ring.
class MicCapture {
public:
    struct Impl;
    static std::optional<MicCapture> create(MicCallback cb, int sample_rate = 48000);
    void start();
    void stop();   // waits until callbacks have actually ceased
    ~MicCapture();
    MicCapture(MicCapture&&) noexcept;
    MicCapture& operator=(MicCapture&&) noexcept;
    MicCapture(const MicCapture&) = delete;
    MicCapture& operator=(const MicCapture&) = delete;
private:
    Impl* impl_ = nullptr;
    explicit MicCapture(Impl*);
};
