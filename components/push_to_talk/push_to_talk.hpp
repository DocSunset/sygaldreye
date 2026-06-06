#pragma once
#include <functional>
#include <string>
#include <vector>

// Called when transcription result is received (or on error).
using TranscriptCallback = std::function<void(std::string_view text)>;

struct PushToTalk {
    void set_companion_url(std::string url);

    // Call from audio thread (real-time safe when not recording; push_back when recording).
    void feed(const float* samples, int frames);

    // Call from main thread.
    void begin_recording();
    void end_recording(TranscriptCallback on_result);  // starts async POST

private:
    std::string companion_url_;
    std::vector<float> buffer_;
    bool recording_    = false;
    int  sample_rate_  = 16000;
};
