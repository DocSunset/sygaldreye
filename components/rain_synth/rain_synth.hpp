#pragma once
#include <cstdint>
#include <string_view>
#include "audio_scene.hpp"
#include "synth_core.hpp"
#include "biquad_filter.hpp"
#include "sygaldry_endpoints.hpp"

struct RainParams {
    float rate        = 200.0f;
    float drop_freq   = 4000.0f;
    float decay_s     = 0.01f;
    float sample_rate = 48000.0f;
};

class RainSynth : public MonoSynth {
public:
    static consteval std::string_view name()          { return "rain_synth"; }
    static consteval std::string_view source_header() { return "components/rain_synth/rain_synth.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/rain_synth/rain_synth.cpp"; }

    struct inputs {
        slider<"rate",      "", float, fp(0.f),    fp(1000.f),  fp(200.f)>  rate;
        slider<"drop freq", "", float, fp(500.f),  fp(10000.f), fp(4000.f)> drop_freq;
        slider<"decay s",   "", float, fp(0.001f), fp(0.5f),    fp(0.01f)>  decay_s;
    } inputs;

    static constexpr int k_max_drops = 64;
    explicit RainSynth(RainParams const& = {});
    void set_params(RainParams const&);
    void operator()(double);
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
