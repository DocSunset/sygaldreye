#pragma once
#include <array>
#include <atomic>
#include "audio_scene.hpp"

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
    explicit ChimeSynth(ChimeParams const& = {});
    void set_params(ChimeParams const&);
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
