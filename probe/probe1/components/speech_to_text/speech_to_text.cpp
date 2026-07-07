// Copyright 2025 Travis West
#include "speech_to_text.hpp"
#include <android/log.h>
#include <string_view>

void SpeechToTextNode::operator()(double /*time_s*/) {
    ptt_.set_companion_url(endpoints.url.get().empty()
        ? "http://192.168.1.1:9090/transcribe" : endpoints.url.get());

    endpoints.bip.value = 0.f;
    bool held = endpoints.record.get() > 0.5f;
    if (held && !recording_) {
        ptt_.begin_recording();
        recording_ = true;
        endpoints.bip.value = 1.f;    // take started
    } else if (!held && recording_) {
        ptt_.pause_recording();      // take ends; audio kept for more takes
        recording_ = false;
        endpoints.bip.value = 0.5f;   // take stopped
        __android_log_print(ANDROID_LOG_INFO, "eyeballs",
                            "stt: take ended, has_audio=%d", int(ptt_.has_audio()));
    }
    if (recording_ && endpoints.audio_in.get().data && endpoints.audio_in.get().frames > 0) {
        ptt_.set_sample_rate(endpoints.audio_in.get().sample_rate);
        ptt_.feed(endpoints.audio_in.get().data, endpoints.audio_in.get().frames);
    }

    bool send_now = endpoints.send.get() > 0.5f;
    if (send_now && !prev_send_ && ptt_.has_audio()) {
        auto pending = pending_;
        ptt_.end_recording([pending](std::string_view t) {
            __android_log_print(ANDROID_LOG_INFO, "eyeballs",
                                "transcript: %.*s", static_cast<int>(t.size()), t.data());
            std::lock_guard<std::mutex> lock(pending->m);
            pending->text  = std::string{t};
            pending->fresh = true;
        });
        recording_ = false;
        endpoints.bip.value = 1.f;
    }
    prev_send_ = send_now;

    endpoints.heard.triggered = false;
    {
        std::lock_guard<std::mutex> lock(pending_->m);
        if (pending_->fresh) {
            endpoints.text.value      = pending_->text;
            endpoints.heard.triggered = true;
            pending_->fresh         = false;
        }
    }

    bool erase_now = endpoints.erase.get() > 0.5f;
    if (erase_now && !prev_erase_) {
        ptt_.erase();
        recording_ = false;
        endpoints.bip.value = 0.5f;
    }
    prev_erase_ = erase_now;

    endpoints.recording.value = recording_ ? 1.f : 0.f;
}
