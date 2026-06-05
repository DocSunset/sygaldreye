#pragma once
#include <array>
#include "audio_scene.hpp"
#include "synth_core.hpp"

enum class CreatureKind { Cricket, Bird, Insect };

struct CreatureParams {
    CreatureKind kind              = CreatureKind::Cricket;
    float carrier_freq             = 4500.0f;
    float fm_index                 = 2.0f;
    float chirp_rate               = 20.0f;
    std::array<float, 8> pitch_contour = {880,1100,990,770,880,1320,1100,880};
    float phrase_duration_s        = 2.0f;
    float sample_rate              = 48000.0f;
};

class CreatureSynth : public MonoSynth {
public:
    explicit CreatureSynth(CreatureParams const& = {});
    void set_params(CreatureParams const&);
    void fill(float* out, int frames) override;

private:
    CreatureParams params_;

    // shared oscillators
    synth::Phasor carrier_;
    synth::Phasor modulator_;
    synth::Adsr   adsr_;

    // cricket
    synth::Phasor group_phasor_;
    float         chirp_accum_    = 0.0f;

    // bird
    int   contour_idx_            = 0;
    float contour_phase_          = 0.0f;

    // insect
    synth::Phasor wing_;
};
