#include "audio_output.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <sys/stat.h>

TEST(AudioOutput, SineWav)
{
    constexpr int   SAMPLE_RATE  = 48000;
    constexpr int   TOTAL_FRAMES = SAMPLE_RATE; // 1 second
    constexpr float FREQ         = 440.0f;
    constexpr float TWO_PI       = 6.28318530f;

    int   frames_written = 0;
    float phase          = 0.0f;
    AudioOutput* ptr     = nullptr;

    auto ao = AudioOutput::create([&](float* out, int frames) {
        for (int i = 0; i < frames; ++i) {
            float s = sinf(phase);
            out[i * 2]     = s;
            out[i * 2 + 1] = s;
            phase = fmodf(phase + TWO_PI * FREQ / SAMPLE_RATE, TWO_PI);
        }
        frames_written += frames;
        if (frames_written >= TOTAL_FRAMES && ptr)
            ptr->stop();
    }, SAMPLE_RATE);

    ASSERT_TRUE(ao.has_value());
    ptr = &*ao;
    ao->start();

    // stop() fires mid-block; file contains whole blocks up to and including
    // the block where stop was triggered: ceil(48000/256)*256 = 48128 frames.
    constexpr int BLOCK = 256;
    constexpr int EXPECTED_FRAMES = ((TOTAL_FRAMES + BLOCK - 1) / BLOCK) * BLOCK;
    constexpr off_t EXPECTED_BYTES = 44 + static_cast<off_t>(EXPECTED_FRAMES) * 2 * 4;

    struct stat st{};
    ASSERT_EQ(stat("audio_output_test.wav", &st), 0);
    EXPECT_EQ(st.st_size, EXPECTED_BYTES);
}
