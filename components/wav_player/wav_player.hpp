// Copyright 2026 Travis West
#pragma once
#include "audio_output.hpp"
#include "sygaldry_endpoints.hpp"
#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// Plays WAV files (TTS replies pushed to /play) and short UI bips through
// its own AAudio stream. file/seq arrive as param edits; bip is a trigger
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
    void ensure_stream();
    bool load_wav(const std::string& path);

    std::unique_ptr<AudioOutput> out_;
    // Audio-thread state: swapped in under a generation counter.
    std::shared_ptr<std::vector<float>> clip_;   // mono 48k
    std::atomic<size_t> clip_pos_{0};
    std::atomic<float>  gain_{0.8f};
    std::atomic<float>  bip_freq_{0.f};
    std::atomic<int>    bip_frames_{0};
    float prev_seq_ = 0.f, prev_bip_ = 0.f;
    float bip_phase_ = 0.f;  // audio-thread only
};
