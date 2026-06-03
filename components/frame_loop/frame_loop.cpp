#include "frame_loop.hpp"
#include <android/log.h>
#include <time.h>

#define LOG(...)  __android_log_print(ANDROID_LOG_INFO,  "eyeballs", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  "eyeballs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)

namespace {
constexpr double kFrameBudgetWarningMs = 9.0;
constexpr int    kFrameDropLogInterval = 100;
}

static double now_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void FrameLoop::run_frame(XrSession session, std::function<FrameLayers(XrTime)> on_render) {
    if (firstFrame_) { LOG("frame loop running"); firstFrame_ = false; }

    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    XrResult r = xrWaitFrame(session, &waitInfo, &frameState);
    if (XR_FAILED(r)) { LOGE("xrWaitFrame failed: %d", (int)r); return; }

    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    r = xrBeginFrame(session, &beginInfo);
    if (XR_FAILED(r)) { LOGE("xrBeginFrame failed: %d", (int)r); return; }

    FrameLayers frame_layers;
    if (frameState.shouldRender && on_render) {
        double render_start = now_sec();
        frame_layers = on_render(frameState.predictedDisplayTime);
        double render_ms = (now_sec() - render_start) * 1000.0;
        if (render_ms > kFrameBudgetWarningMs) {
            LOGW("render over budget: %.2f ms", render_ms);
        }
    }

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime          = frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    if (frame_layers.count > 0U) {
        endInfo.layerCount = frame_layers.count;
        endInfo.layers     = frame_layers.layers.data();
    }
    r = xrEndFrame(session, &endInfo);
    ++frame_count_;
    if (XR_FAILED(r)) {
        ++frame_drops_;
        double t2 = now_sec();
        if (t2 - lastEndErr_ >= 2.0) { LOGE("xrEndFrame failed: %d", (int)r); lastEndErr_ = t2; }
        return;
    }

    if (frame_count_ % kFrameDropLogInterval == 0 && frame_drops_ > 0) {
        LOG("frame drops: %d in last %d frames", frame_drops_, kFrameDropLogInterval);
        frame_drops_ = 0;
    }

    double t = now_sec();
    if (t - lastHeartbeat_ >= 5.0) { LOG("frame loop heartbeat"); lastHeartbeat_ = t; }
}
