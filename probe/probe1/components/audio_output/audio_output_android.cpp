#include "audio_output.hpp"
#include <aaudio/AAudio.h>
#include <android/log.h>
#include <atomic>
#include <memory>

struct AudioOutput::Impl {
    AAudioStream*    stream   = nullptr;
    AudioCallback    callback;
    std::atomic_bool dead{false};
};

static aaudio_data_callback_result_t data_callback(
    AAudioStream*, void* user, void* audio_data, int32_t frames)
{
    auto* impl = static_cast<AudioOutput::Impl*>(user);
    impl->callback(static_cast<float*>(audio_data), frames);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

// A disconnected AAudio stream (route change, device idle) never calls
// back again — flag it so the owner recreates (zombie-stream silence,
// kanban block_swap_poison.md).
static void error_callback(AAudioStream*, void* user, aaudio_result_t error)
{
    __android_log_print(ANDROID_LOG_WARN, "eyeballs",
                        "audio_output: stream error %s — flagging dead",
                        AAudio_convertResultToText(error));
    static_cast<AudioOutput::Impl*>(user)->dead = true;
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
    AAudioStreamBuilder_setErrorCallback(builder, error_callback, impl);

    aaudio_result_t r = AAudioStreamBuilder_openStream(builder, &impl->stream);
    AAudioStreamBuilder_delete(builder);

    if (r != AAUDIO_OK) { delete impl; return {}; }
    return AudioOutput{impl};
}

void AudioOutput::start() { AAudioStream_requestStart(impl_->stream); }
bool AudioOutput::dead() const { return impl_ && impl_->dead.load(); }
void AudioOutput::stop() {
    AAudioStream_requestStop(impl_->stream);
    // requestStop is async — wait for callbacks to actually cease before
    // the destructor closes and frees (same use-after-free class as the
    // mic_capture teardown crash, 2026-06-12).
    aaudio_stream_state_t st = AAudioStream_getState(impl_->stream);
    for (int i = 0; i < 10 && st != AAUDIO_STREAM_STATE_STOPPED &&
                    st != AAUDIO_STREAM_STATE_CLOSED; ++i)
        AAudioStream_waitForStateChange(impl_->stream, st, &st, 100'000'000);
}

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
