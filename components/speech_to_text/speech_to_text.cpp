// Copyright 2025 Travis West
#include "speech_to_text.hpp"
#include <android/log.h>
#include <string_view>

void SpeechToTextNode::operator()(double /*time_s*/) {
    ptt_.set_companion_url(inputs.url.value.empty()
        ? "http://192.168.1.1:9090/transcribe" : inputs.url.value);

    outputs.bip.value = 0.f;
    bool held = inputs.record.value > 0.5f;
    if (held && !recording_) {
        ptt_.begin_recording();
        recording_ = true;
        outputs.bip.value = 1.f;    // take started
    } else if (!held && recording_) {
        ptt_.pause_recording();      // take ends; audio kept for more takes
        recording_ = false;
        outputs.bip.value = 0.5f;   // take stopped
    }
    if (recording_ && inputs.audio_in.value.data && inputs.audio_in.value.frames > 0)
        ptt_.feed(inputs.audio_in.value.data, inputs.audio_in.value.frames);

    bool send_now = inputs.send.value > 0.5f;
    if (send_now && !prev_send_ && ptt_.has_audio()) {
        ptt_.end_recording([](std::string_view t) {
            __android_log_print(ANDROID_LOG_INFO, "eyeballs",
                                "transcript: %.*s", static_cast<int>(t.size()), t.data());
        });
        recording_ = false;
        outputs.bip.value = 1.f;
    }
    prev_send_ = send_now;

    bool erase_now = inputs.erase.value > 0.5f;
    if (erase_now && !prev_erase_) {
        ptt_.erase();
        recording_ = false;
        outputs.bip.value = 0.5f;
    }
    prev_erase_ = erase_now;

    outputs.recording.value = recording_ ? 1.f : 0.f;
}
