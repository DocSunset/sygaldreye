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
                 {"from":"pack.audio","to":"un.audio"}]})");
    ASSERT_TRUE(f.g);
    for (int i = 0; i < 5; ++i) f.frame();
    const AudioBuffer* packed = f.audio("pack.audio");
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
                 {"from":"pack.audio","to":"lp.audio"},
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
    const AudioBuffer* live = f.audio("m.audio");
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
    auto it = vals2.find("m.audio");
    ASSERT_NE(it, vals2.end());
    const AudioBuffer* after = std::get_if<AudioBuffer>(&it->second);
    ASSERT_TRUE(after);
    EXPECT_EQ(after->frames, 0);  // unconnected in<T> = absent, not stale
}
