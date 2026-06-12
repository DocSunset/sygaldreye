// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <string>
#include <string_view>

// Plays WAV files (TTS replies pushed to /play) and short UI bips through
// THE process audio engine — it owns no hardware (Pd model: one stream
// owner per process). file/seq arrive as param edits; bip is a trigger
// input wired from speech_to_text.bip.
struct WavPlayerNode {
    static consteval std::string_view name() { return "wav_player"; }
    static consteval std::string_view source_header() { return "components/wav_player/wav_player.hpp"; }

    struct inputs {
        ::text<"file"> file;
        slider<"seq", "", float, fp(0.f), fp(1e9f), fp(0.f)> seq;   // change → (re)play file
        slider<"bip", "", float, fp(0.f), fp(1.f),  fp(0.f)> bip;   // >0 → beep (1=hi, 0.5=lo)
        slider<"gain","", float, fp(0.f), fp(2.f),  fp(0.8f)> gain;
    } inputs;
    struct outputs {
        port<"playing", float> playing;
    } outputs;

    void operator()(double);

private:
    float  prev_seq_ = 0.f, prev_bip_ = 0.f;
    double playing_until_ = 0.0;
};
