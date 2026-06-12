#include "mic_capture.hpp"
#include <aaudio/AAudio.h>
#include <android/log.h>
#include <cstdint>
#include <vector>
#define MLOG(...) __android_log_print(ANDROID_LOG_INFO, "eyeballs", __VA_ARGS__)

struct MicCapture::Impl {
    AAudioStream* stream = nullptr;
    bool          is_i16 = false;     // AAudio format is a REQUEST; device may
    std::vector<int16_t> i16buf;      // deliver I16 regardless — convert here
};

std::optional<MicCapture> MicCapture::create(int sample_rate)
{
    auto* impl = new Impl{};

    AAudioStreamBuilder* builder = nullptr;
    if (AAudio_createStreamBuilder(&builder) != AAUDIO_OK) { delete impl; return {}; }

    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setSampleRate(builder, sample_rate);
    // No data callback: pull mode (see header). NONE mode: LOW_LATENCY
    // input contends with the system mic path on Quest and intermittently
    // delivers nothing.
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_NONE);

    aaudio_result_t r = AAudioStreamBuilder_openStream(builder, &impl->stream);
    AAudioStreamBuilder_delete(builder);

    if (r != AAUDIO_OK) {
        MLOG("mic_capture: openStream failed: %s", AAudio_convertResultToText(r));
        delete impl;
        return {};
    }
    aaudio_format_t fmt = AAudioStream_getFormat(impl->stream);
    impl->is_i16 = (fmt == AAUDIO_FORMAT_PCM_I16);
    MLOG("mic_capture: pull-mode input open at %d Hz, format %s",
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
    // requestStop is async; wait so close() never races a device thread.
    aaudio_stream_state_t st = AAudioStream_getState(impl_->stream);
    for (int i = 0; i < 10 && st != AAUDIO_STREAM_STATE_STOPPED &&
                    st != AAUDIO_STREAM_STATE_CLOSED; ++i)
        AAudioStream_waitForStateChange(impl_->stream, st, &st, 100'000'000);
}

int MicCapture::read(float* dst, int max_frames) {
    if (!impl_ || !impl_->stream || max_frames <= 0) return 0;
    if (!impl_->is_i16) {
        aaudio_result_t n = AAudioStream_read(impl_->stream, dst, max_frames, 0);
        return n > 0 ? n : 0;
    }
    impl_->i16buf.resize(std::size_t(max_frames));
    aaudio_result_t n = AAudioStream_read(impl_->stream, impl_->i16buf.data(),
                                          max_frames, 0);
    if (n <= 0) return 0;
    for (int i = 0; i < n; ++i)
        dst[i] = float(impl_->i16buf[std::size_t(i)]) / 32768.f;
    return n;
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
