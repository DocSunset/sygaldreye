#pragma once
#include <cstdint>
#include "audio_scene.hpp"
#include "synth_core.hpp"
#include "biquad_filter.hpp"

struct RainParams {
    float rate        = 200.0f;
    float drop_freq   = 4000.0f;
    float decay_s     = 0.01f;
    float sample_rate = 48000.0f;
};

class RainSynth : public MonoSynth {
public:
    static constexpr int k_max_drops = 64;
    explicit RainSynth(RainParams const& = {});
    void set_params(RainParams const&);
    void fill(float* out, int frames) override;
private:
    struct Drop {
        synth::Adsr          env;
        synth::BiquadFilter  lp;
        uint32_t             noise_state;
        bool                 active;
    };
    RainParams params_;
    Drop       drops_[k_max_drops];
    uint32_t   spawn_rng_;
};
