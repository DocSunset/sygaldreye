#pragma once
#include <optional>

// Microphone capture in PULL mode: the stream has NO data callback — the
// owner drains it with non-blocking read() from whatever thread ticks it.
// One audio thread per process: a second AAudio callback thread starved
// the OUTPUT stream on Quest (silent/distorted synths — see kanban
// block_swap_poison.md), and a callback outliving its buffers corrupted
// the heap on teardown. Pull mode removes both classes.
class MicCapture {
public:
    struct Impl;
    static std::optional<MicCapture> create(int sample_rate = 48000);
    void start();
    void stop();
    // Non-blocking: returns frames actually read (mono float32), 0 if none.
    int  read(float* dst, int max_frames);
    ~MicCapture();
    MicCapture(MicCapture&&) noexcept;
    MicCapture& operator=(MicCapture&&) noexcept;
    MicCapture(const MicCapture&) = delete;
    MicCapture& operator=(const MicCapture&) = delete;
private:
    Impl* impl_ = nullptr;
    explicit MicCapture(Impl*);
};
