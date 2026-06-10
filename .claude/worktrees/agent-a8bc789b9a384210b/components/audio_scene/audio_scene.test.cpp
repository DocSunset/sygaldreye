// Copyright 2025 Travis West
#include "audio_scene.hpp"
#include <gtest/gtest.h>
#include <cmath>

class SineSource : public MonoSynth {
public:
    explicit SineSource(float freq = 440.0f, float sr = 48000.0f)
        : phase_(0.0f), inc_(2.0f * 3.14159265f * freq / sr) {}

    void fill(float* out, int frames) override {
        for (int i = 0; i < frames; ++i) {
            out[i] = std::sin(phase_);
            phase_ += inc_;
            if (phase_ > 2.0f * 3.14159265f) phase_ -= 2.0f * 3.14159265f;
        }
    }

private:
    float phase_;
    float inc_;
};

TEST(AudioScene, StereoAsymmetricFromRight)
{
    AudioScene scene;
    SineSource sine;

    AudioSource src;
    src.world_position = {2.0f, 0.0f, 0.0f};
    src.gain           = 1.0f;
    src.synth          = &sine;
    scene.add_source(src);
    scene.set_listener({0, 0, 0}, Eigen::Quaternionf::Identity());

    static constexpr int kFrames = 256;
    float buf[kFrames * 2] = {};
    scene.mix(buf, kFrames);

    double sum_l = 0.0, sum_r = 0.0;
    for (int i = 0; i < kFrames; ++i) {
        sum_l += buf[i * 2]     * buf[i * 2];
        sum_r += buf[i * 2 + 1] * buf[i * 2 + 1];
    }
    double rms_l = std::sqrt(sum_l / kFrames);
    double rms_r = std::sqrt(sum_r / kFrames);

    EXPECT_GT(rms_l + rms_r, 0.0) << "output should be non-zero";
    EXPECT_NE(rms_l, rms_r)       << "should be stereo asymmetric";
}
