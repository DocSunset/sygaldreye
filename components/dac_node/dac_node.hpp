// Copyright 2026 Travis West
#pragma once
#include <cmath>
#include <string_view>
#include <vector>

#include "sygaldry_endpoints.hpp"

// The audio output bus, as a node. Everything reachable upstream of a dac
// through audio edges forms the BLOCK region, ticked by the audio callback.
// The dac gains its input into an owned stereo-ready buffer and meters RMS;
// it does NOT own the hardware. AudioEngine owns the one AudioOutput stream,
// and audio_region SUMS every dac's `audio_out` into it (multiple dacs
// sum-mix, Pd-style: mono duplicated to both ears, stereo added) — or drives
// the offline pump when no device exists.
//
// KERNEL-EXTRACTION / CONFORMABILITY EXCEPTION: dac is the region's output
// terminal, not a per-sample Lift kernel. conformability.md lists it among
// the non-lifted boundary nodes — replicating a dac is meaningless (there is
// one device), and the meaningful multi-dac semantics, summing, already lives
// in audio_region. Its per-sample body is a gain + an RMS reduction (a
// reduce, not element-wise) regardless.
struct DacNode {
    static consteval std::string_view name() { return "dac"; }
    static consteval std::string_view source_header() { return "components/dac_node/dac_node.hpp"; }
    // resource_holder here means "unliftable terminal", NOT hardware
    // ownership (AudioEngine owns the stream): dac is the region's summing
    // output, so the executor must never clone it.
    static constexpr int lift_kind() { return lift::resource_holder; }

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
