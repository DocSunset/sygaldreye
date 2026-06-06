#include "mic_capture.hpp"
#include <aaudio/AAudio.h>

struct MicCapture::Impl {
    AAudioStream* stream   = nullptr;
    MicCallback   callback;
};

static aaudio_data_callback_result_t data_callback(
    AAudioStream*, void* user, void* audio_data, int32_t frames)
{
    auto* impl = static_cast<MicCapture::Impl*>(user);
    impl->callback(static_cast<const float*>(audio_data), frames);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

std::optional<MicCapture> MicCapture::create(MicCallback cb, int sample_rate)
{
    auto* impl = new Impl{};
    impl->callback = std::move(cb);

    AAudioStreamBuilder* builder = nullptr;
    if (AAudio_createStreamBuilder(&builder) != AAUDIO_OK) { delete impl; return {}; }

    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setSampleRate(builder, sample_rate);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setDataCallback(builder, data_callback, impl);

    aaudio_result_t r = AAudioStreamBuilder_openStream(builder, &impl->stream);
    AAudioStreamBuilder_delete(builder);

    if (r != AAUDIO_OK) { delete impl; return {}; }
    return MicCapture{impl};
}

void MicCapture::start() { AAudioStream_requestStart(impl_->stream); }
void MicCapture::stop()  { AAudioStream_requestStop(impl_->stream); }

MicCapture::MicCapture(Impl* impl) : impl_{impl} {}

MicCapture::~MicCapture()
{
    if (!impl_) return;
    stop();
    AAudioStream_close(impl_->stream);
    delete impl_;
}

MicCapture::MicCapture(MicCapture&& o) noexcept : impl_{o.impl_} { o.impl_ = nullptr; }

MicCapture& MicCapture::operator=(MicCapture&& o) noexcept
{
    if (this != &o) { this->~MicCapture(); impl_ = o.impl_; o.impl_ = nullptr; }
    return *this;
}
