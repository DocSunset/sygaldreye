// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <cmath>
#include <string_view>
#include <vector>

// The audio device, as a node: one dac per peer owns the output stream
// (edge_executor.design.md — advertised = available). Everything reachable
// upstream of a dac through audio edges forms the BLOCK region, ticked by
// the audio callback. The dac itself just gains its input into an owned
// stereo-ready buffer; the audio_region scheduler delivers `out` to the
// device (or to the offline pump when no device exists).
struct DacNode {
    static consteval std::string_view name() { return "dac"; }
    static consteval std::string_view source_header() { return "components/dac_node/dac_node.hpp"; }

    struct inputs {
        port<"audio", AudioBuffer> audio;
        slider<"gain", "", float, fp(0.f), fp(1.f), fp(0.8f)> gain;
    } inputs;

    struct outputs {
        port<"out",   AudioBuffer> out;
        port<"level", float>       level;  // block RMS, snapshots to frame
    } outputs;

    void operator()(double) {
        const AudioBuffer& in = inputs.audio.value;
        buf_.assign(in.data && in.frames > 0 ? std::size_t(in.frames) : 0, 0.f);
        float sq = 0.f;
        for (std::size_t i = 0; i < buf_.size(); ++i) {
            buf_[i] = in.data[i] * inputs.gain.value;
            sq += buf_[i] * buf_[i];
        }
        outputs.out.value   = AudioBuffer{buf_.data(), int(buf_.size()), 1,
                                          in.sample_rate};
        outputs.level.value = buf_.empty() ? 0.f
            : std::sqrt(sq / float(buf_.size()));
    }

private:
    std::vector<float> buf_;
};
