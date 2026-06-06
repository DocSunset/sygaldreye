#pragma once
#include <atomic>
#include <cstdint>
#include <string_view>
#include "audio_scene.hpp"
#include "synth_core.hpp"
#include "biquad_filter.hpp"
#include "sygaldry_endpoints.hpp"

struct EngineParams {
    float rpm         = 800.0f;
    int   cylinders   = 4;
    float roughness   = 0.3f;
    float load        = 0.5f;
    float sample_rate = 48000.0f;
};

class EngineSynth : public MonoSynth {
public:
    static consteval std::string_view name()          { return "engine_synth"; }
    static consteval std::string_view source_header() { return "components/engine_synth/engine_synth.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/engine_synth/engine_synth.cpp"; }

    struct inputs {
        slider<"rpm",       "", float, fp(100.f), fp(8000.f), fp(800.f)> rpm;
        slider<"roughness", "", float, fp(0.f),   fp(1.f),    fp(0.3f)>  roughness;
        slider<"load",      "", float, fp(0.f),   fp(1.f),    fp(0.5f)>  load;
    } inputs;

    static constexpr int k_max_harmonics = 8;
    explicit EngineSynth(EngineParams const& = {});
    void set_params(EngineParams const&);
    void operator()(double);
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
