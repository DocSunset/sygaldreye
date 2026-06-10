#pragma once
#include <array>
#include <string_view>
#include "audio_scene.hpp"
#include "synth_core.hpp"
#include "sygaldry_endpoints.hpp"

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
    static consteval std::string_view name()          { return "creature_synth"; }
    static consteval std::string_view source_header() { return "components/creature_synth/creature_synth.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/creature_synth/creature_synth.cpp"; }

    struct inputs {
        slider<"carrier_freq",       "", float, fp(100.f), fp(10000.f), fp(4500.f)> carrier_freq;
        slider<"fm_index",           "", float, fp(0.f),   fp(20.f),    fp(2.f)>    fm_index;
        slider<"chirp_rate",         "", float, fp(0.5f),  fp(100.f),   fp(20.f)>   chirp_rate;
        slider<"phrase_duration_s",  "", float, fp(0.1f),  fp(10.f),    fp(2.f)>    phrase_duration_s;
    } inputs;

    explicit CreatureSynth(CreatureParams const& = {});
    void set_params(CreatureParams const&);
    void operator()(double);
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
