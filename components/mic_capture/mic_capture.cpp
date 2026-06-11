#include "mic_capture.hpp"
#include <aaudio/AAudio.h>
#include <android/log.h>
#define MLOG(...) __android_log_print(ANDROID_LOG_INFO, "eyeballs", __VA_ARGS__)

struct MicCapture::Impl {
    AAudioStream* stream   = nullptr;
    MicCallback   callback;
};

static aaudio_data_callback_result_t data_callback(
    AAudioStream*, void* user, void* audio_data, int32_t frames)
{
    static int logged = 0;
    if (logged < 3) { MLOG("mic_capture: callback delivering %d frames", frames); ++logged; }
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
    // LOW_LATENCY input contends with the system mic path on Quest and
    // intermittently delivers nothing; default/shared mode is reliable.
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_NONE);
    AAudioStreamBuilder_setDataCallback(builder, data_callback, impl);

    aaudio_result_t r = AAudioStreamBuilder_openStream(builder, &impl->stream);
    AAudioStreamBuilder_delete(builder);

    if (r != AAUDIO_OK) {
        MLOG("mic_capture: openStream failed: %s", AAudio_convertResultToText(r));
        delete impl;
        return {};
    }
    MLOG("mic_capture: input stream open at %d Hz", sample_rate);
    return MicCapture{impl};
}

void MicCapture::start() {
    aaudio_result_t r = AAudioStream_requestStart(impl_->stream);
    if (r != AAUDIO_OK)
        MLOG("mic_capture: requestStart failed: %s", AAudio_convertResultToText(r));
}
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
