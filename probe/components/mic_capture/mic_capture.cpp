#include "mic_capture.hpp"
#include <aaudio/AAudio.h>
#include <android/log.h>
#include <cstdint>
#include <vector>
#define MLOG(...) __android_log_print(ANDROID_LOG_INFO, "eyeballs", __VA_ARGS__)

struct MicCapture::Impl {
    AAudioStream* stream = nullptr;
    MicCallback   callback;
    bool          is_i16 = false;     // AAudio format is a REQUEST; device may
    std::vector<float> conv;          // deliver I16 regardless — convert here
};

static aaudio_data_callback_result_t data_callback(
    AAudioStream*, void* user, void* audio_data, int32_t frames)
{
    auto* impl = static_cast<MicCapture::Impl*>(user);
    if (impl->is_i16) {
        impl->conv.resize(std::size_t(frames));
        const int16_t* in = static_cast<const int16_t*>(audio_data);
        for (int i = 0; i < frames; ++i)
            impl->conv[std::size_t(i)] = float(in[i]) / 32768.f;
        impl->callback(impl->conv.data(), frames);
    } else {
        impl->callback(static_cast<const float*>(audio_data), frames);
    }
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
    aaudio_format_t fmt = AAudioStream_getFormat(impl->stream);
    impl->is_i16 = (fmt == AAUDIO_FORMAT_PCM_I16);
    MLOG("mic_capture: input open at %d Hz, format %s",
         AAudioStream_getSampleRate(impl->stream),
         impl->is_i16 ? "I16 -> converting" : "FLOAT");
    return MicCapture{impl};
}

void MicCapture::start() {
    aaudio_result_t r = AAudioStream_requestStart(impl_->stream);
    if (r != AAUDIO_OK)
        MLOG("mic_capture: requestStart failed: %s", AAudio_convertResultToText(r));
}

void MicCapture::stop() {
    AAudioStream_requestStop(impl_->stream);
    // requestStop is async. Wait until callbacks have actually ceased:
    // closing (then freeing the callback's captures) with the stream
    // still running is a use-after-free (device heap corruption,
    // 2026-06-12).
    aaudio_stream_state_t st = AAudioStream_getState(impl_->stream);
    for (int i = 0; i < 10 && st != AAUDIO_STREAM_STATE_STOPPED &&
                    st != AAUDIO_STREAM_STATE_CLOSED; ++i)
        AAudioStream_waitForStateChange(impl_->stream, st, &st, 100'000'000);
}

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
