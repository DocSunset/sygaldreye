#pragma once
#include <atomic>
#include <cstdint>
#include <string_view>
#include "audio_scene.hpp"
#include "biquad_filter.hpp"
#include "sygaldry_endpoints.hpp"

struct FireParams {
    float intensity    = 1.0f;
    float crackle_rate = 40.0f;
    float sample_rate  = 48000.0f;
};

class FireSynth : public MonoSynth {
public:
    static consteval std::string_view name()          { return "fire_synth"; }
    static consteval std::string_view source_header() { return "components/fire_synth/fire_synth.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/fire_synth/fire_synth.cpp"; }

    struct inputs {
        slider<"intensity",    "", float, fp(0.f), fp(5.f),   fp(1.f)>  intensity;
        slider<"crackle_rate", "", float, fp(0.f), fp(200.f), fp(40.f)> crackle_rate;
    } inputs;

    static constexpr int k_max_crackles = 16;
    explicit FireSynth(FireParams const& = {});
    void set_params(FireParams const&);
    void operator()(double);
    void fill(float* out, int frames) override;
private:
    struct Crackle {
        uint32_t            noise_state;
        synth::BiquadFilter hp;
        float               env;
        float               decay;
        bool                active;
    };

    std::atomic<FireParams> params_;
    uint32_t                spawn_rng_{0xdeadbeef};
    uint32_t                roar_noise_{0xcafef00d};
    uint32_t                hiss_noise_{0x12345678};
    synth::BiquadFilter     roar_bp_;
    synth::BiquadFilter     hiss_hp_;
    float                   freq_lfo_phase_{0.0f};
    float                   amp_lfo_phase_{0.0f};
    Crackle                 crackles_[k_max_crackles]{};
};
