#pragma once
#include <functional>
#include <optional>

// Called from audio thread with captured mono float32 samples.
// Must be real-time safe.
using MicCallback = std::function<void(const float* samples, int frames)>;

class MicCapture {
public:
    struct Impl;
    static std::optional<MicCapture> create(MicCallback, int sample_rate = 16000);
    void start();
    void stop();
    ~MicCapture();
    MicCapture(MicCapture&&) noexcept;
    MicCapture& operator=(MicCapture&&) noexcept;
    MicCapture(const MicCapture&) = delete;
    MicCapture& operator=(const MicCapture&) = delete;
private:
    Impl* impl_ = nullptr;
    explicit MicCapture(Impl*);
};
