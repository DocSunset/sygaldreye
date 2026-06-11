// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <atomic>
#include <memory>
#include <string>
#include <string_view>

// Speech to text on the PC peer: transcribes `file` (WAV, delivered by the
// core's POST /transcribe ingress) and forwards the transcript as a /param
// event to `target_node` at `target_url` — the companion's /transcribe
// endpoint, as a live-editable graph node. Fires when seq changes.
// Transcription + forwarding run on the worker region. The transcript
// forwards over HTTP rather than an edge because text payloads are still
// param-only (edge_executor open question).
struct WhisperNode {
    static consteval std::string_view name() { return "whisper_stt"; }
    static consteval std::string_view source_header() { return "components/whisper_node/whisper_node.hpp"; }

    struct inputs {
        ::text<"file">        file;
        // empty → "companion/.venv/bin/python companion/transcribe_cli.py"
        ::text<"command">     command;
        // empty → "http://127.0.0.1:8930/param"
        ::text<"target_url">  target_url;
        ::text<"target_node"> target_node;  // empty → "claude"
        slider<"seq", "", float, fp(0.f), fp(1e9f), fp(0.f)> seq;
    } inputs;

    struct outputs {
        port<"sent", float> sent;  // pulses 1 when a transcript forwarded
    } outputs;

    void operator()(double);

private:
    struct Shared {
        std::atomic_bool  sent{false};
        std::atomic<long> fwd_seq{0};
    };
    std::shared_ptr<Shared> sh_ = std::make_shared<Shared>();
    float prev_seq_ = 0.f;
};
