#pragma once
#include <atomic>
#include <string_view>
#include "audio_scene.hpp"
#include "biquad_filter.hpp"
#include "sygaldry_endpoints.hpp"

struct AtmosParams {
    float wind_speed  = 1.0f;
    float base_freq   = 300.0f;
    float brightness  = 0.5f;
    float sample_rate = 48000.0f;
};

class AtmosSynth : public MonoSynth {
public:
    static consteval std::string_view name()          { return "atmos_synth"; }
    static consteval std::string_view source_header() { return "components/atmos_synth/atmos_synth.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/atmos_synth/atmos_synth.cpp"; }

    struct inputs {
        slider<"wind_speed", "", float, fp(0.f),   fp(10.f),   fp(1.f)>   wind_speed;
        slider<"base_freq",  "", float, fp(50.f),  fp(2000.f), fp(300.f)> base_freq;
        slider<"brightness", "", float, fp(0.f),   fp(1.f),    fp(0.5f)>  brightness;
    } inputs;

    explicit AtmosSynth(AtmosParams const& = {});
    void set_params(AtmosParams const&);
    void operator()(double);
    void fill(float* out, int frames) override;

private:
    std::atomic<AtmosParams> params_;
    uint32_t                 noise_state_ = 0x12345678u;
    float                    freq_lfo_phase_ = 0.0f;
    float                    amp_lfo_phase_  = 0.0f;
    synth::BiquadFilter      wind_filter_;
    synth::BiquadFilter      whistle_filter_;
};
