#pragma once
#include <atomic>
#include <cstdint>
#include "audio_scene.hpp"
#include "synth_core.hpp"
#include "biquad_filter.hpp"

struct EngineParams {
    float rpm         = 800.0f;
    int   cylinders   = 4;
    float roughness   = 0.3f;
    float load        = 0.5f;
    float sample_rate = 48000.0f;
};

class EngineSynth : public MonoSynth {
public:
    static constexpr int k_max_harmonics = 8;
    explicit EngineSynth(EngineParams const& = {});
    void set_params(EngineParams const&);
    void fill(float* out, int frames) override;

private:
    std::atomic<EngineParams> params_;
    float                     sample_rate_  = 48000.0f;
    float                     current_rpm_  = 800.0f;
    synth::Phasor              harmonics_[k_max_harmonics];
    synth::Phasor              mod_phasor_;
    synth::BiquadFilter        noise_filter_;
    uint32_t                   noise_state_ = 0x12345678u;
};
