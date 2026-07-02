// Copyright 2026 Travis West
#include "ugens.hpp"

#include <gtest/gtest.h>

#include <cmath>

#include "component_registry.hpp"
#include "eyeballs_node_abi.hpp"
#include "osc_node.hpp"
#include "signal_graph.hpp"

namespace {
struct Fixture {
    ComponentRegistry reg;
    std::unique_ptr<Graph> g;
    double t = 0.0;
    explicit Fixture(const char* json) {
        reg.register_builtin(make_descriptor<OscNode>());
        reg.register_builtin(make_descriptor<McPackNode>());
        reg.register_builtin(make_descriptor<McUnpackNode>());
        reg.register_builtin(make_descriptor<BiquadNode>());
        reg.register_builtin(make_descriptor<MetroNode>());
        reg.register_builtin(make_descriptor<AdsrNode>());
        reg.register_builtin(make_descriptor<VcaNode>());
        reg.register_builtin(make_descriptor<MixNode>());
        reg.register_builtin(make_descriptor<NoiseNode>());
        reg.register_builtin(make_descriptor<PercNode>());
        reg.register_builtin(make_descriptor<GrainCloudNode>());
        g = parse_graph(json, reg);
    }
    void frame() {
        t += 1.0 / 60.0;
        tick_graph(*g, t);
    }
    std::unordered_map<std::string, PortValue> vals;
    const AudioBuffer* audio(const char* key) {
        vals = snapshot_values(*g);
        auto it = vals.find(key);
        return it == vals.end() ? nullptr : std::get_if<AudioBuffer>(&it->second);
    }
};

// Planar view over a test-owned vector: ch contiguous blocks of frames.
AudioBuffer planar(const std::vector<float>& v, int ch, int sr = 48000) {
    return {v.data(), int(v.size()) / ch, ch, sr};
}

float rms(const AudioBuffer& b, int c) {
    float s = 0.f;
    for (int i = 0; i < b.frames; ++i) {
        float v = ugen_detail::at(b, i, c);
        s += v * v;
    }
    return b.frames ? std::sqrt(s / float(b.frames)) : 0.f;
}
}  // namespace

TEST(Multichannel, PackCarriesChannelsAndUnpackPeelsThem) {
    Fixture f(R"({"nodes":[
        {"id":"a","type":"osc","params":{"freq":100,"amp":1}},
        {"id":"b","type":"osc","params":{"freq":100,"amp":0}},
        {"id":"pack","type":"mc_pack","params":{}},
        {"id":"un","type":"mc_unpack","params":{}}],
        "edges":[{"from":"a.audio","to":"pack.in1"},
                 {"from":"b.audio","to":"pack.in2"},
                 {"from":"pack.audio_out","to":"un.audio"}]})");
    ASSERT_TRUE(f.g);
    for (int i = 0; i < 5; ++i) f.frame();
    const AudioBuffer* packed = f.audio("pack.audio_out");
    ASSERT_TRUE(packed);
    EXPECT_EQ(packed->channels, 2);
    const AudioBuffer* o1 = f.audio("un.out1");
    const AudioBuffer* o2 = f.audio("un.out2");
    ASSERT_TRUE(o1 && o2);
    EXPECT_GT(rms(*o1, 0), 0.4f);   // loud sine survived in channel 1
    EXPECT_LT(rms(*o2, 0), 0.01f);  // silent sine stayed silent in channel 2
}

TEST(Multichannel, ProcessorsAdaptPerChannelState) {
    // Two-channel signal through one biquad: per-channel filter state, no
    // cross-channel bleed (silent channel stays silent through the filter).
    Fixture f(R"({"nodes":[
        {"id":"a","type":"osc","params":{"freq":500,"amp":1}},
        {"id":"b","type":"osc","params":{"freq":500,"amp":0}},
        {"id":"pack","type":"mc_pack","params":{}},
        {"id":"lp","type":"biquad","params":{"freq":2000}},
        {"id":"un","type":"mc_unpack","params":{}}],
        "edges":[{"from":"a.audio","to":"pack.in1"},
                 {"from":"b.audio","to":"pack.in2"},
                 {"from":"pack.audio_out","to":"lp.audio"},
                 {"from":"lp.audio_out","to":"un.audio"}]})");
    ASSERT_TRUE(f.g);
    for (int i = 0; i < 5; ++i) f.frame();
    const AudioBuffer* filtered = f.audio("lp.audio_out");
    ASSERT_TRUE(filtered);
    EXPECT_EQ(filtered->channels, 2);
    EXPECT_GT(rms(*f.audio("un.out1"), 0), 0.4f);
    EXPECT_LT(rms(*f.audio("un.out2"), 0), 0.01f);
}

TEST(Adsr, GateSignalDrivesEnvelope) {
    // metro gate (audio square) → CV adsr: envelope rises during the gate
    // and falls after it.
    Fixture f(R"({"nodes":[
        {"id":"clk","type":"metro","params":{"rate":2,"duty":0.5}},
        {"id":"env","type":"adsr","params":{"attack":0.01,"decay":0.05,"sustain":0.8,"release":0.05}}],
        "edges":[{"from":"clk.gate_out","to":"env.gate"}]})");
    ASSERT_TRUE(f.g);
    float peak = 0.f, trough = 1.f;
    for (int i = 0; i < 90; ++i) {  // 1.5 s: three gate cycles
        f.frame();
        const AudioBuffer* env = f.audio("env.audio_out");
        ASSERT_TRUE(env);
        if (i > 30) {  // past the first cycle
            float r = rms(*env, 0);
            peak = std::max(peak, r);
            trough = std::min(trough, r);
        }
    }
    EXPECT_GT(peak, 0.5f);    // sustained near sustain level while gated
    EXPECT_LT(trough, 0.1f);  // released between gates
}

TEST(Generators, ShellsProduceSoundThroughGenerate) {
    // Guards the kernel_extraction.md generator-shell rewrite: noise, perc
    // (gate-held), and grain_cloud all emit non-silent mono audio through
    // synth::Gen::generate.
    Fixture f(R"({"nodes":[
        {"id":"nz","type":"noise","params":{"amp":0.5,"color":0}},
        {"id":"pc","type":"perc","params":{"gate":1,"attack":0.005,"sustain":0.8}},
        {"id":"gc","type":"grain_cloud","params":{"rate":800,"amp":0.8,"decay":0.05}}]
        ,"edges":[]})");
    ASSERT_TRUE(f.g);
    float nz_rms = 0.f, pc_peak = 0.f, gc_rms = 0.f;
    for (int i = 0; i < 30; ++i) {  // 0.5 s: perc attacks to sustain
        f.frame();
        nz_rms = std::max(nz_rms, rms(*f.audio("nz.audio"), 0));
        pc_peak = std::max(pc_peak, rms(*f.audio("pc.audio"), 0));
        gc_rms = std::max(gc_rms, rms(*f.audio("gc.audio"), 0));
    }
    EXPECT_GT(nz_rms, 0.1f);   // white noise at amp 0.5
    EXPECT_GT(pc_peak, 0.3f);  // envelope reaches the sustain plateau
    EXPECT_GT(gc_rms, 0.01f);  // stochastic grains fire and sum
}

TEST(EndpointsV6Combiners, DeletedProducerLeavesNoStaleDrone) {
    // THE drone regression (2026-06-12): a v6 mix fed by a legacy osc must
    // fall structurally silent when the producer is deleted — never replay
    // a stale buffer view.
    Fixture f(R"({"nodes":[
        {"id":"a","type":"osc","params":{"freq":200,"amp":1}},
        {"id":"m","type":"mix","params":{}}],
        "edges":[{"from":"a.audio","to":"m.in1"}]})");
    ASSERT_TRUE(f.g);
    for (int i = 0; i < 5; ++i) f.frame();
    const AudioBuffer* live = f.audio("m.audio_out");
    ASSERT_TRUE(live);
    EXPECT_GT(rms(*live, 0), 0.4f);  // slot path delivers legacy audio

    auto g2 = parse_graph(
        R"({"nodes":[
        {"id":"m","type":"mix","params":{}}],
        "edges":[]})",
        f.reg);
    ASSERT_TRUE(g2);
    migrate_graph(*g2, *f.g);  // the SAME mix instance survives
    tick_graph(*g2, f.t += 1.0 / 60.0);
    tick_graph(*g2, f.t += 1.0 / 60.0);
    auto vals2 = snapshot_values(*g2);
    auto it = vals2.find("m.audio_out");
    ASSERT_NE(it, vals2.end());
    const AudioBuffer* after = std::get_if<AudioBuffer>(&it->second);
    ASSERT_TRUE(after);
    EXPECT_EQ(after->frames, 0);  // unconnected in<T> = absent, not stale
}

// ── direct node-shell tests (v6 endpoints, no graph) ─────────────────────────

TEST(LiftState, ChannelGrowthPreservesSurvivorState) {
    // 2ch impulses into a delay, then grow to 3ch BEFORE the echo lands:
    // surviving channels' tails must play out; the new channel is fresh.
    DelayNode d;
    d.endpoints.time.fallback = 0.001f;  // 48 samples at 48k
    d.endpoints.feedback.fallback = 0.f;
    d.endpoints.wet.fallback = 1.f;  // output = echo only
    std::vector<float> b1(2 * 32, 0.f);
    b1[0] = 1.f;    // ch0 impulse
    b1[32] = 0.5f;  // ch1 impulse
    AudioBuffer in1 = planar(b1, 2);
    d.endpoints.audio.src = &in1;
    d(0.0);
    std::vector<float> b2(3 * 32, 0.f);
    AudioBuffer in2 = planar(b2, 3);
    d.endpoints.audio.src = &in2;
    d(0.0);  // echo due at global frame 48 = local frame 16
    const AudioBuffer& out = d.endpoints.audio_out.value;
    ASSERT_EQ(out.channels, 3);
    EXPECT_FLOAT_EQ(ugen_detail::at(out, 16, 0), 1.f);
    EXPECT_FLOAT_EQ(ugen_detail::at(out, 16, 1), 0.5f);
    for (int i = 0; i < 32; ++i) EXPECT_FLOAT_EQ(ugen_detail::at(out, i, 2), 0.f);
}

TEST(Biquad, CoefficientsFollowBufferSampleRate) {
    // Same digital sine (1/24 cycle per sample), tagged 24k vs 48k. At 24k
    // it sits AT the 1 kHz cutoff (−3 dB); at 48k it's 2 kHz = 2× cutoff.
    // A hardcoded coefficient rate would make both outputs identical.
    auto rms_at = [](int sr) {
        BiquadNode n;
        n.endpoints.freq.fallback = 1000.f;
        std::vector<float> v(4800);
        for (int i = 0; i < 4800; ++i) v[std::size_t(i)] = std::sin(6.28318530f * float(i) / 24.f);
        AudioBuffer in = planar(v, 1, sr);
        n.endpoints.audio.src = &in;
        n(0.0);
        const AudioBuffer& out = n.endpoints.audio_out.value;
        float s = 0.f;
        for (int i = 2400; i < 4800; ++i) s += out.data[i] * out.data[i];  // skip transient
        return std::sqrt(s / 2400.f);
    };
    float at_cutoff = rms_at(24000), above_cutoff = rms_at(48000);
    EXPECT_NEAR(at_cutoff, 0.5f, 0.05f);  // 0.707 (−3 dB) × 0.707 (sine rms)
    EXPECT_LT(above_cutoff, at_cutoff * 0.6f);
}

TEST(Osc, WaveformsPinnedSampleExact) {
    // Pins the per-sample phase wrap (4be6455): at 12 kHz / 48 kHz the phase
    // wraps every 4 samples — a block-end fmod would diverge from sample 4 on.
    auto pin = [](float wave_sel, float (*shape)(float)) {
        OscNode o;
        o.endpoints.freq.fallback = 12000.f;
        o.endpoints.amp.fallback = 1.f;
        o.endpoints.wave.fallback = wave_sel;
        o(1.0);  // first tick: 1/60 s → 800 frames
        const AudioBuffer& b = o.endpoints.audio.value;
        ASSERT_GE(b.frames, 64);
        float phase = 0.f;
        const float step = 6.28318530f * 12000.f / 48000.f;
        for (int i = 0; i < 64; ++i) {
            EXPECT_FLOAT_EQ(b.data[i], shape(phase)) << "wave " << wave_sel << " sample " << i;
            phase += step;
            if (phase >= 6.28318530f) phase -= 6.28318530f;
        }
    };
    pin(0.f, synth::sine);
    pin(1.f, synth::sawtooth);
    pin(2.f, synth::square);
    pin(3.f, synth::triangle);
}

TEST(NodeShells, ShaperMatchesTanhReference) {
    ShaperNode n;
    n.endpoints.drive.fallback = 2.f;
    std::vector<float> v{0.f, 0.25f, -0.5f, 1.f};
    AudioBuffer in = planar(v, 1);
    n.endpoints.audio.src = &in;
    n(0.0);
    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ(
            n.endpoints.audio_out.value.data[i],
            std::tanh(v[std::size_t(i)] * 2.f) / std::tanh(2.f));
}

TEST(NodeShells, VcaAppliesGainAndBroadcastsMonoEnv) {
    VcaNode n;
    n.endpoints.gain.fallback = 0.5f;
    std::vector<float> a(2 * 4, 1.f);                         // ch0 = 1
    for (int i = 0; i < 4; ++i) a[std::size_t(4 + i)] = 2.f;  // ch1 = 2
    std::vector<float> e{0.f, 1.f, 0.5f, 0.25f};              // mono env
    AudioBuffer av = planar(a, 2), ev = planar(e, 1);
    n.endpoints.audio.src = &av;
    n.endpoints.env.src = &ev;
    n(0.0);
    const AudioBuffer& out = n.endpoints.audio_out.value;
    ASSERT_EQ(out.channels, 2);
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 4; ++i)
            EXPECT_FLOAT_EQ(ugen_detail::at(out, i, c), float(c + 1) * 0.5f * e[std::size_t(i)]);
}

TEST(NodeShells, SampleHoldSharesOneClockAcrossChannels) {
    // Catches iteration-order regressions in the shared-clock mask: every
    // channel must sample at the SAME frame indices (23, 47) — a per-channel
    // (c-outer) clock would shift the second channel's sample points.
    SampleHoldNode n;
    n.endpoints.rate.fallback = 2000.f;  // period = 24 frames at 48k
    std::vector<float> v(2 * 60);
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 60; ++i) v[std::size_t(c * 60 + i)] = float(100 * c + i);
    AudioBuffer in = planar(v, 2);
    n.endpoints.audio.src = &in;
    n(0.0);
    const AudioBuffer& out = n.endpoints.audio_out.value;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 60; ++i) {
            float held = i < 23 ? 0.f : float(100 * c + (i < 47 ? 23 : 47));
            EXPECT_FLOAT_EQ(ugen_detail::at(out, i, c), held) << "c=" << c << " i=" << i;
        }
}

TEST(NodeShells, SlewLimitsStepPerSecond) {
    SlewNode n;
    n.endpoints.target.fallback = 10.f;
    n.endpoints.rate.fallback = 60.f;  // units/s → 1 unit per 1/60 s tick
    n(1.0);                            // first tick: dt defaults to 1/60
    EXPECT_FLOAT_EQ(n.endpoints.out.value, 1.f);
    n(1.0 + 1.0 / 60.0);
    EXPECT_FLOAT_EQ(n.endpoints.out.value, 2.f);
    n.endpoints.target.fallback = 0.f;
    n.endpoints.rate.fallback = 100000.f;  // slam: reaches target in one tick
    n(1.0 + 2.0 / 60.0);
    EXPECT_FLOAT_EQ(n.endpoints.out.value, 0.f);
}
