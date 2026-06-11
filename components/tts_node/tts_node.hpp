// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <atomic>
#include <memory>
#include <string>
#include <string_view>

// Speaks text into a peer's ears: synthesizes `message` (MeloTTS by
// default) and POSTs the WAV to `play_url` (a peer's /play → its speaker
// node). The companion's /tts endpoint, as a live-editable graph node.
// Fires when seq changes (params persist; same pattern as claude_tmux).
// Synthesis + upload run on the worker region.
struct TtsNode {
    static consteval std::string_view name() { return "tts"; }
    static consteval std::string_view source_header() { return "components/tts_node/tts_node.hpp"; }

    struct inputs {
        ::text<"message">  message;
        // empty → "companion/.venv-melotts/bin/python companion/tts_cli.py"
        ::text<"command">  command;
        // empty → "http://127.0.0.1:8080/play" (the device peer)
        ::text<"play_url"> play_url;
        slider<"seq", "", float, fp(0.f), fp(1e9f), fp(0.f)> seq;
    } inputs;

    struct outputs {
        port<"spoken", float> spoken;  // pulses 1 when a wav was delivered
    } outputs;

    void operator()(double);

private:
    struct Shared { std::atomic_bool spoken{false}; };
    std::shared_ptr<Shared> sh_ = std::make_shared<Shared>();
    float prev_seq_ = 0.f;
};
