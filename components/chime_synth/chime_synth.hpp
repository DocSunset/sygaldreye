#pragma once
#include <array>
#include <atomic>
#include <string_view>
#include "audio_scene.hpp"
#include "sygaldry_endpoints.hpp"

struct Mode {
    float freq      = 440.0f;
    float amplitude = 1.0f;
    float decay_s   = 1.0f;
};

struct ChimeParams {
    std::array<Mode, 8> modes{};
    int   mode_count   = 4;
    float strike_rate  = 0.0f;
    float strike_gain  = 1.0f;
    float sample_rate  = 48000.0f;
};

class ChimeSynth : public MonoSynth {
public:
    static consteval std::string_view name()          { return "chime_synth"; }
    static consteval std::string_view source_header() { return "components/chime_synth/chime_synth.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/chime_synth/chime_synth.cpp"; }

    struct inputs {
        slider<"strike rate", "", float, fp(0.f), fp(10.f), fp(0.f)>  strike_rate;
        slider<"strike gain", "", float, fp(0.f), fp(2.f),  fp(1.f)>  strike_gain;
    } inputs;

    explicit ChimeSynth(ChimeParams const& = {});
    void set_params(ChimeParams const&);
    void operator()(double);
    void strike(float gain = 1.0f);
    void fill(float* out, int frames) override;
private:
    ChimeParams              params_;
    std::array<float, 8>     phase_;
    std::array<float, 8>     envelope_;
    std::array<float, 8>     decay_coeff_;
    float                    poisson_acc_;
    std::atomic<float>       pending_strike_gain_;

    void apply_strike(float gain);
    void recompute_coeffs();
};

ChimeParams wind_chime_params();
ChimeParams bell_params();
ChimeParams robot_click_params();
