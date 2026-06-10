#pragma once
#include <atomic>
#include <cstdint>
#include "audio_scene.hpp"
#include "synth_core.hpp"
#include "biquad_filter.hpp"

struct WaterParams {
    float flow_speed  = 1.0f;
    float wave_rate   = 0.2f;
    float wave_height = 0.5f;
    float brightness  = 0.5f;
    float sample_rate = 48000.0f;
};

class WaterSynth : public MonoSynth {
public:
    explicit WaterSynth(WaterParams const& = {});
    void set_params(WaterParams const&);
    void fill(float* out, int frames) override;

private:
    static constexpr int k_splash_count = 4;

    struct Splash {
        synth::BiquadFilter lp;
        uint32_t            noise_state = 1;
        float               env         = 0.0f;
        float               decay       = 0.0f;
        bool                active      = false;
    };

    std::atomic<WaterParams> params_;
    synth::BiquadFilter      bp_;
    synth::Lfo               lfo_a_;
    synth::Lfo               lfo_b_;
    synth::Phasor            wave_phasor_;
    uint32_t                 noise_state_ = 0x12345678u;
    float                    wave_env_    = 0.0f;
    bool                     wave_above_  = false;
    Splash                   splashes_[k_splash_count];
};
