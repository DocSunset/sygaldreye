// Copyright 2026 Travis West
// SDL2 backend: desktop (ALSA/Pulse via SDL) and the browser (Emscripten's
// SDL2 → Web Audio). Same callback contract as the Android AAudio backend.
#include "audio_output.hpp"
#include <SDL.h>
#include <cstdio>

struct AudioOutput::Impl {
    AudioCallback     callback;
    SDL_AudioDeviceID dev = 0;
};

namespace {
void sdl_cb(void* userdata, Uint8* stream, int len) {
    auto* impl = static_cast<AudioOutput::Impl*>(userdata);
    impl->callback(reinterpret_cast<float*>(stream), len / int(2 * sizeof(float)));
}
} // namespace

std::optional<AudioOutput> AudioOutput::create(AudioCallback cb, int sample_rate) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        std::fprintf(stderr, "audio_output: SDL audio init failed: %s\n", SDL_GetError());
        return std::nullopt;
    }
    auto* impl = new Impl{};
    impl->callback = std::move(cb);
    SDL_AudioSpec want{}, have{};
    want.freq     = sample_rate;
    want.format   = AUDIO_F32SYS;
    want.channels = 2;
    want.samples  = 256;
    want.callback = sdl_cb;
    want.userdata = impl;
    impl->dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (impl->dev == 0) {
        std::fprintf(stderr, "audio_output: open failed: %s\n", SDL_GetError());
        delete impl;
        return std::nullopt;
    }
    return AudioOutput{impl};
}

void AudioOutput::start() { if (impl_) SDL_PauseAudioDevice(impl_->dev, 0); }
void AudioOutput::stop()  { if (impl_) SDL_PauseAudioDevice(impl_->dev, 1); }
bool AudioOutput::dead() const { return false; }  // SDL handles route changes

AudioOutput::AudioOutput(Impl* impl) : impl_{impl} {}

AudioOutput::~AudioOutput() {
    if (!impl_) return;
    SDL_CloseAudioDevice(impl_->dev);
    delete impl_;
}

AudioOutput::AudioOutput(AudioOutput&& o) noexcept : impl_{o.impl_} { o.impl_ = nullptr; }

AudioOutput& AudioOutput::operator=(AudioOutput&& o) noexcept {
    if (this != &o) { this->~AudioOutput(); impl_ = o.impl_; o.impl_ = nullptr; }
    return *this;
}
