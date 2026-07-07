// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <mutex>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// Sample playback as a block-region ugen: emits its clip through an audio
// edge like any source (wire to dac, spatialize, spectrogram…). Replaces
// wav_player's private stream + hand-rolled atomics: the loader runs on
// the worker region, the swap is a shared_ptr, the boundaries are the
// region's mappings. /play targets it (file/seq/bip params — id "speaker"
// by convention) exactly as before.
struct SamplePlayerNode {
    static consteval std::string_view name() { return "sample_player"; }
    static consteval std::string_view source_header() { return "components/sample_player/sample_player.hpp"; }
    static constexpr int lift_kind() { return lift::resource_holder; }

    struct endpoints {
        normalled_in<std::string> file;
        normalled_in<float, fp(0.f), fp(1e9f), fp(0.f)> seq;  // change → (re)play
        normalled_in<float, fp(0.f), fp(1.f),  fp(0.f)> bip;  // >0 → beep (1=hi, .5=lo)
        normalled_in<float, fp(0.f), fp(2.f),  fp(0.8f)> gain;
        out<AudioBuffer> audio;
        out<float>       playing;
    } endpoints;

    void operator()(double time_s);

private:
    // Worker loads land here; the block tick adopts under a brief lock
    // (same contention tolerance as the latch mappings).
    struct Shared {
        std::mutex m;
        std::shared_ptr<std::vector<float>> incoming;
    };
    std::shared_ptr<Shared> sh_ = std::make_shared<Shared>();
    std::shared_ptr<std::vector<float>> clip_;
    std::vector<float> buf_;
    std::size_t pos_ = 0;
    double prev_t_ = 0.0;
    float  prev_seq_ = 0.f, prev_bip_ = 0.f, bip_phase_ = 0.f;
    int    bip_frames_ = 0;
    float  bip_freq_ = 1100.f;
};
