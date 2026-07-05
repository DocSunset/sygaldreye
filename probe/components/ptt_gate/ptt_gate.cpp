// Copyright 2025 Travis West
#include "ptt_gate.hpp"

void PttGate::operator()(double /*time_s*/) {
    bool was_open = open_;
    if (endpoints.open.get()  > 0.5f) open_ = true;
    if (endpoints.close.get() > 0.5f) open_ = false;

    endpoints.opened.value    = (!was_open &&  open_) ? 1.f : 0.f;
    endpoints.closed.value    = ( was_open && !open_) ? 1.f : 0.f;
    endpoints.is_open.value   = open_ ? 1.f : 0.f;
    endpoints.audio_out.value = open_ ? endpoints.audio_in.get()
                                      : AudioBuffer{nullptr, 0, 1, 16000};
}
