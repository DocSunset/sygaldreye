#pragma once
#include <functional>
#include <string>
#include <vector>

// Called when transcription result is received (or on error).
using TranscriptCallback = std::function<void(std::string_view text)>;

struct PushToTalk {
    void set_companion_url(std::string url);
    void set_sample_rate(int sr) { sample_rate_ = sr; }

    // Call from audio thread (real-time safe when not recording; push_back when recording).
    void feed(const float* samples, int frames);

    // Call from main thread. Takes accumulate: begin appends to the held
    // buffer, pause stops appending, send posts everything and clears,
    // erase discards.
    void begin_recording();
    void pause_recording();
    void erase();
    void end_recording(TranscriptCallback on_result);  // pause + send
    bool has_audio() const { return !buffer_.empty(); }

private:
    std::string companion_url_;
    std::vector<float> buffer_;
    bool recording_    = false;
    int  sample_rate_  = 16000;
};
