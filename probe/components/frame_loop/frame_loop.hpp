#pragma once
#include <openxr/openxr.h>
#include <array>
#include <functional>

struct FrameLayers {
    std::array<const XrCompositionLayerBaseHeader*, 16> layers{};
    uint32_t count = 0;
};

struct FrameLoop {
private:
    bool   firstFrame_    = true;
    double lastHeartbeat_ = 0.0;
    double lastEndErr_    = 0.0;
    int    frame_count_   = 0;
    int    frame_drops_   = 0;

public:
    void run_frame(XrSession session, std::function<FrameLayers(XrTime)> on_render);
};
