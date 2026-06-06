#pragma once
#include <atomic>
#include <cstdint>
#include <string_view>
#include "audio_scene.hpp"
#include "synth_core.hpp"
#include "biquad_filter.hpp"
#include "sygaldry_endpoints.hpp"

struct WaterSynthParams {
    float flow_speed  = 1.0f;
    float wave_rate   = 0.2f;
    float wave_height = 0.5f;
    float brightness  = 0.5f;
    float sample_rate = 48000.0f;
};

class WaterSynth : public MonoSynth {
public:
    static consteval std::string_view name()          { return "water_synth"; }
    static consteval std::string_view source_header() { return "components/water_synth/water_synth.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/water_synth/water_synth.cpp"; }

    struct inputs {
        slider<"flow speed",  "", float, fp(0.f), fp(10.f), fp(1.f)>  flow_speed;
        slider<"wave rate",   "", float, fp(0.f), fp(2.f),  fp(0.2f)> wave_rate;
        slider<"wave height", "", float, fp(0.f), fp(2.f),  fp(0.5f)> wave_height;
        slider<"brightness",  "", float, fp(0.f), fp(1.f),  fp(0.5f)> brightness;
    } inputs;

    explicit WaterSynth(WaterSynthParams const& = {});
    void set_params(WaterSynthParams const&);
    void operator()(double);
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

    std::atomic<WaterSynthParams> params_;
    synth::BiquadFilter      bp_;
    synth::Lfo               lfo_a_;
    synth::Lfo               lfo_b_;
    synth::Phasor            wave_phasor_;
    uint32_t                 noise_state_ = 0x12345678u;
    float                    wave_env_    = 0.0f;
    bool                     wave_above_  = false;
    Splash                   splashes_[k_splash_count];
};
