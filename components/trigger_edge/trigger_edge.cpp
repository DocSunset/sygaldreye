// Copyright 2025 Travis West
#include "trigger_edge.hpp"

void TriggerEdge::operator()(double) {
    float t   = endpoints.trigger.get();
    float thr = endpoints.threshold.get();
    bool  cur = t > thr, was = prev_ > thr;
    endpoints.press.value   = (!was && cur)  ? 1.f : 0.f;
    endpoints.release.value = (was  && !cur) ? 1.f : 0.f;
    endpoints.held.value    = cur ? 1.f : 0.f;
    prev_ = t;
}
