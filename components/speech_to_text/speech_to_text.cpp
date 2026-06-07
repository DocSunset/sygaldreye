// Copyright 2025 Travis West
#include "speech_to_text.hpp"
#include <android/log.h>
#include <string_view>

void SpeechToTextNode::operator()(double /*time_s*/) {
    ptt_.set_companion_url(companion_url);

    if (inputs.begin.value > 0.5f && !recording_) {
        ptt_.begin_recording();
        recording_ = true;
    }
    if (recording_ && inputs.audio_in.value.data && inputs.audio_in.value.frames > 0)
        ptt_.feed(inputs.audio_in.value.data, inputs.audio_in.value.frames);
    if (inputs.finalize.value > 0.5f && recording_) {
        ptt_.end_recording([](std::string_view t) {
            __android_log_print(ANDROID_LOG_INFO, "eyeballs",
                                "transcript: %.*s", static_cast<int>(t.size()), t.data());
        });
        recording_ = false;
    }
}

std::string to_json(const SpeechToTextNode& node) {
    std::string out = "{\"companion_url\":\"";
    out += node.companion_url;
    out += "\"}";
    return out;
}

void from_json(SpeechToTextNode& node, std::string_view json) {
    constexpr std::string_view key = "\"companion_url\":\"";
    auto pos = json.find(key);
    if (pos == std::string_view::npos) return;
    pos += key.size();
    auto end = json.find('"', pos);
    if (end == std::string_view::npos) return;
    node.companion_url = std::string(json.substr(pos, end - pos));
}
