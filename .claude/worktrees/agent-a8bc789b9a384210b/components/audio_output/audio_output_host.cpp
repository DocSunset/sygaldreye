#include "audio_output.hpp"
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <vector>

static constexpr int FRAMES_PER_BLOCK = 256;

struct WavHeader {
    char     riff[4]         = {'R','I','F','F'};
    uint32_t chunk_size      = 0;
    char     wave[4]         = {'W','A','V','E'};
    char     fmt[4]          = {'f','m','t',' '};
    uint32_t fmt_size        = 16;
    uint16_t audio_format    = 3; // IEEE float
    uint16_t channels        = 2;
    uint32_t sample_rate     = 48000;
    uint32_t byte_rate       = 384000;
    uint16_t block_align     = 8;
    uint16_t bits_per_sample = 32;
    char     data[4]         = {'d','a','t','a'};
    uint32_t data_size       = 0;
};
static_assert(sizeof(WavHeader) == 44);

struct AudioOutput::Impl {
    AudioCallback     callback;
    int               sample_rate;
    std::atomic<bool> running{false};
    std::vector<float> samples;
};

std::optional<AudioOutput> AudioOutput::create(AudioCallback cb, int sample_rate)
{
    auto* impl = new Impl{};
    impl->callback    = std::move(cb);
    impl->sample_rate = sample_rate;
    return AudioOutput{impl};
}

void AudioOutput::start()
{
    impl_->running = true;
    impl_->samples.clear();

    std::vector<float> block(FRAMES_PER_BLOCK * 2);
    while (impl_->running) {
        impl_->callback(block.data(), FRAMES_PER_BLOCK);
        impl_->samples.insert(impl_->samples.end(), block.begin(), block.end());
    }

    uint32_t n_frames   = static_cast<uint32_t>(impl_->samples.size() / 2);
    uint32_t data_bytes = n_frames * 8u;

    WavHeader hdr{};
    hdr.sample_rate = static_cast<uint32_t>(impl_->sample_rate);
    hdr.byte_rate   = static_cast<uint32_t>(impl_->sample_rate) * 8u;
    hdr.chunk_size  = 36 + data_bytes;
    hdr.data_size   = data_bytes;

    FILE* f = fopen("audio_output_test.wav", "wb");
    if (!f) return;
    fwrite(&hdr, sizeof(hdr), 1, f);
    fwrite(impl_->samples.data(), sizeof(float), impl_->samples.size(), f);
    fclose(f);
}

void AudioOutput::stop() { impl_->running = false; }

AudioOutput::AudioOutput(Impl* impl) : impl_{impl} {}

AudioOutput::~AudioOutput()
{
    if (!impl_) return;
    delete impl_;
}

AudioOutput::AudioOutput(AudioOutput&& o) noexcept : impl_{o.impl_} { o.impl_ = nullptr; }

AudioOutput& AudioOutput::operator=(AudioOutput&& o) noexcept
{
    if (this != &o) { this->~AudioOutput(); impl_ = o.impl_; o.impl_ = nullptr; }
    return *this;
}
