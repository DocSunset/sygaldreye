#include "audio_output.hpp"
#include <aaudio/AAudio.h>
#include <memory>

struct AudioOutput::Impl {
    AAudioStream*  stream   = nullptr;
    AudioCallback  callback;
};

static aaudio_data_callback_result_t data_callback(
    AAudioStream*, void* user, void* audio_data, int32_t frames)
{
    auto* impl = static_cast<AudioOutput::Impl*>(user);
    impl->callback(static_cast<float*>(audio_data), frames);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

std::optional<AudioOutput> AudioOutput::create(AudioCallback cb, int sample_rate)
{
    auto* impl = new Impl{};
    impl->callback = std::move(cb);

    AAudioStreamBuilder* builder = nullptr;
    if (AAudio_createStreamBuilder(&builder) != AAUDIO_OK) { delete impl; return {}; }

    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setChannelCount(builder, 2);
    AAudioStreamBuilder_setSampleRate(builder, sample_rate);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, 256);
    AAudioStreamBuilder_setDataCallback(builder, data_callback, impl);

    aaudio_result_t r = AAudioStreamBuilder_openStream(builder, &impl->stream);
    AAudioStreamBuilder_delete(builder);

    if (r != AAUDIO_OK) { delete impl; return {}; }
    return AudioOutput{impl};
}

void AudioOutput::start() { AAudioStream_requestStart(impl_->stream); }
void AudioOutput::stop()  { AAudioStream_requestStop(impl_->stream); }

AudioOutput::AudioOutput(Impl* impl) : impl_{impl} {}

AudioOutput::~AudioOutput()
{
    if (!impl_) return;
    stop();
    AAudioStream_close(impl_->stream);
    delete impl_;
}

AudioOutput::AudioOutput(AudioOutput&& o) noexcept : impl_{o.impl_} { o.impl_ = nullptr; }

AudioOutput& AudioOutput::operator=(AudioOutput&& o) noexcept
{
    if (this != &o) { this->~AudioOutput(); impl_ = o.impl_; o.impl_ = nullptr; }
    return *this;
}
