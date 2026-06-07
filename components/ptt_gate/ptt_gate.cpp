// Copyright 2025 Travis West
#include "ptt_gate.hpp"

void PttGate::operator()(double /*time_s*/) {
    bool was_open = open_;
    if (inputs.open.value  > 0.5f) open_ = true;
    if (inputs.close.value > 0.5f) open_ = false;

    outputs.opened.value   = (!was_open &&  open_) ? 1.f : 0.f;
    outputs.closed.value   = ( was_open && !open_) ? 1.f : 0.f;
    outputs.is_open.value  = open_ ? 1.f : 0.f;
    outputs.audio_out.value = open_ ? inputs.audio_in.value
                                    : AudioBuffer{nullptr, 0, 1, 16000};
}
