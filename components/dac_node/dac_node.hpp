// Copyright 2026 Travis West
#pragma once
#include <cmath>
#include <string_view>
#include <vector>

#include "sygaldry_endpoints.hpp"

// The audio device, as a node: one dac per peer owns the output stream
// (edge_executor.design.md — advertised = available). Everything reachable
// upstream of a dac through audio edges forms the BLOCK region, ticked by
// the audio callback. The dac itself just gains its input into an owned
// stereo-ready buffer; the audio_region scheduler delivers `out` to the
// device (or to the offline pump when no device exists).
//
// KERNEL-EXTRACTION / CONFORMABILITY EXCEPTION: the dac is a resource
// holder (it owns the output stream), so it is never lifted — lifting a
// subgraph containing one is an error (conformability.md). Its per-sample
// loop is a gain + RMS reduction over an already-shaped buffer, not a
// liftable kernel.
struct DacNode {
    static consteval std::string_view name() { return "dac"; }
    static consteval std::string_view source_header() { return "components/dac_node/dac_node.hpp"; }

    // endpoints v6: an unconnected audio input is structurally silent.
    struct endpoints {
        in<AudioBuffer> audio;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.8f)> gain;
        out<AudioBuffer> audio_out;
        out<float> level;  // block RMS, snapshots to frame
    } endpoints;

    void operator()(double) {
        const AudioBuffer in = endpoints.audio.get();
        int ch = std::max(1, std::min(2, in.channels));
        std::size_t samples =
            in.data && in.frames > 0 ? std::size_t(in.frames) * std::size_t(ch) : 0;
        buf_.assign(samples, 0.f);
        float g = endpoints.gain.get();
        float sq = 0.f;
        for (std::size_t i = 0; i < samples; ++i) {
            buf_[i] = in.data[i] * g;
            sq += buf_[i] * buf_[i];
        }
        endpoints.audio_out.value = AudioBuffer{buf_.data(), int(samples) / ch, ch, in.sample_rate};
        endpoints.level.value = samples == 0 ? 0.f : std::sqrt(sq / float(samples));
    }

private:
    std::vector<float> buf_;
};
