// Copyright 2025 Travis West
#include "trigger_edge.hpp"

void TriggerEdge::operator()(double) {
    float t   = inputs.trigger.value;
    float thr = inputs.threshold.value;
    bool  cur = t > thr, was = prev_ > thr;
    outputs.press.value   = (!was && cur)  ? 1.f : 0.f;
    outputs.release.value = (was  && !cur) ? 1.f : 0.f;
    outputs.held.value    = cur ? 1.f : 0.f;
    prev_ = t;
}
