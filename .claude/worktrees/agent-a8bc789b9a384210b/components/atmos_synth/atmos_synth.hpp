#pragma once
#include <atomic>
#include "audio_scene.hpp"
#include "biquad_filter.hpp"

struct AtmosParams {
    float wind_speed  = 1.0f;
    float base_freq   = 300.0f;
    float brightness  = 0.5f;
    float sample_rate = 48000.0f;
};

class AtmosSynth : public MonoSynth {
public:
    explicit AtmosSynth(AtmosParams const& = {});
    void set_params(AtmosParams const&);
    void fill(float* out, int frames) override;

private:
    std::atomic<AtmosParams> params_;
    uint32_t                 noise_state_ = 0x12345678u;
    float                    freq_lfo_phase_ = 0.0f;
    float                    amp_lfo_phase_  = 0.0f;
    synth::BiquadFilter      wind_filter_;
    synth::BiquadFilter      whistle_filter_;
};
