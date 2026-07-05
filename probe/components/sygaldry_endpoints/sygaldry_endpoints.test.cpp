// Copyright 2025 Travis West
#include "sygaldry_endpoints.hpp"

#include <gtest/gtest.h>

#include <Eigen/Core>
#include <string_view>

TEST(SygaldryEndpoints, InUnwiredReadsCompileTimeDefault) {
    in<float, fp(2.5f)> i;
    EXPECT_FLOAT_EQ(i.get(), 2.5f);
    float v = 7.f;
    i.src = &v;
    EXPECT_FLOAT_EQ(i.get(), 7.f);
}

TEST(SygaldryEndpoints, InEigenDefaultsAreInitialized) {
    in<Eigen::Vector3f> v3;
    EXPECT_FLOAT_EQ(v3.get().norm(), 0.f);  // Zero, not garbage
    in<Eigen::Quaternionf> q;
    EXPECT_FLOAT_EQ(q.get().norm(), 1.f);  // Identity
    in<Eigen::Matrix4f> m;
    EXPECT_TRUE(m.get().isApprox(Eigen::Matrix4f::Identity()));
}

TEST(SygaldryEndpoints, InStreamAbsentWhenUnwired) {
    in<AudioBuffer> a;
    EXPECT_EQ(a.get().data, nullptr);
    EXPECT_EQ(a.get().frames, 0);
}

TEST(SygaldryEndpoints, NormalledFallbackAndOverride) {
    normalled_in<float, fp(0.f), fp(1.f), fp(0.5f)> n;
    EXPECT_FLOAT_EQ(n.min(), 0.f);
    EXPECT_FLOAT_EQ(n.max(), 1.f);
    EXPECT_FLOAT_EQ(n.get(), 0.5f);  // init
    n.fallback = 0.8f;
    EXPECT_FLOAT_EQ(n.get(), 0.8f);  // persisted param
    float wired = 0.1f;
    n.src = &wired;
    EXPECT_FLOAT_EQ(n.get(), 0.1f);  // edge overrides
    n.src = nullptr;
    EXPECT_FLOAT_EQ(n.get(), 0.8f);  // normalled back
}

TEST(SygaldryEndpoints, NormalledString) {
    normalled_in<std::string> t;
    EXPECT_TRUE(t.get().empty());
    t.fallback = "hello";
    EXPECT_EQ(t.get(), "hello");
}

TEST(SygaldryEndpoints, CvInAttenuverter) {
    cv_in<float> c;
    EXPECT_FLOAT_EQ(c.get(), 0.f);  // offset, unwired
    c.offset = 1.f;
    c.slope = -2.f;
    float mod = 3.f;
    c.src = &mod;
    EXPECT_FLOAT_EQ(c.get(), 1.f - 2.f * 3.f);
}

TEST(SygaldryEndpoints, OutOwnsInitializedStorage) {
    out<float> f;
    EXPECT_FLOAT_EQ(f.value, 0.f);
    out<Eigen::Vector3f> v;
    EXPECT_FLOAT_EQ(v.value.norm(), 0.f);  // not Eigen garbage
    out<Eigen::Quaternionf> q;
    EXPECT_FLOAT_EQ(q.value.norm(), 1.f);
}

TEST(SygaldryEndpoints, EventsDefaultUntriggered) {
    event_in i;
    EXPECT_FALSE(i.triggered);
    event_out o;
    EXPECT_FALSE(o.triggered);
}

TEST(SygaldryEndpoints, SpanAxesCarryIdentityNotHeuristic) {
    // A 2-channel planar buffer and a 2-frame stereo block have the SAME
    // counts; only the named axes tell them apart (conformability.md). The
    // executor must NOT guess from "which dimension is larger".
    float d[4] = {0, 0, 0, 0};
    Span planar_audio{d, 2, 2, Axis::Channel, Axis::Time};
    Span two_positions{d, 2, 2, Axis::Item, Axis::Cell};
    EXPECT_EQ(planar_audio.rows, two_positions.rows);  // identical counts
    EXPECT_EQ(planar_audio.cols, two_positions.cols);
    EXPECT_NE(planar_audio.row_axis, two_positions.row_axis);  // told apart by name
    EXPECT_EQ(planar_audio.row_axis, Axis::Channel);
    EXPECT_EQ(planar_audio.col_axis, Axis::Time);
    EXPECT_STREQ(axis_name(planar_audio.row_axis), "channel");
    EXPECT_STREQ(axis_name(two_positions.col_axis), "cell");
}

TEST(SygaldryEndpoints, SpanDefaultsToItemCell) {
    Span s{nullptr, 0, 0};  // existing N×M producers stay frame-leading
    EXPECT_EQ(s.row_axis, Axis::Item);
    EXPECT_EQ(s.col_axis, Axis::Cell);
}

TEST(SygaldryEndpoints, ShapeConceptsClassify) {
    static_assert(V6Input<in<float>>);
    static_assert(V6Input<normalled_in<float>>);
    static_assert(V6Normalled<normalled_in<float>>);
    static_assert(!V6Normalled<in<float>>);
    static_assert(V6Cv<cv_in<float>>);
    static_assert(V6Output<out<float>>);
    static_assert(!V6Output<in<float>>);
    static_assert(V6EventIn<event_in> && V6EventOut<event_out>);
    SUCCEED();
}

TEST(SygaldryEndpoints, WholeIsAConnectionInputThatOptsOutOfLifting) {
    whole<Span> w;
    static_assert(V6Input<whole<Span>>);  // wires like any input
    static_assert(V6Whole<whole<Span>>);  // but marked whole-array
    static_assert(!V6Whole<in<Span>>);    // a plain Span input still lifts
    EXPECT_EQ(w.get().data, nullptr);     // unwired ⇒ empty
    Span s{nullptr, 3, 3};
    w.src = &s;
    EXPECT_EQ(w.get().rows, 3);  // wired ⇒ the whole payload
}
