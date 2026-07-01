// Copyright 2026 Travis West
#include "spatialize_node.hpp"

#include <cmath>

void SpatializeNode::operator()(double) {
    const AudioBuffer in = endpoints.audio.get();
    int n = (in.data && in.frames > 0) ? in.frames : 0;
    buf_.assign(std::size_t(n) * 2, 0.f);

    if (n > 0) {
        // Source direction in listener space; distance drives rolloff/air.
        Eigen::Vector3f rel = endpoints.pos.get() - endpoints.listener_pos.get();
        Eigen::Vector3f dir = endpoints.listener_rot.get().conjugate() * rel;
        float dist = std::max(0.05f, dir.norm());
        sp_.set_position(dir / dist, dist);
        sp_.process(in.data, buf_.data(), n);
        float g = endpoints.gain.get();
        if (g != 1.f)
            for (auto& s : buf_) s *= g;
    }

    float sl = 0.f, sr = 0.f;
    for (int i = 0; i < n; ++i) {  // planar: [L block][R block]
        sl += buf_[std::size_t(i)] * buf_[std::size_t(i)];
        sr += buf_[std::size_t(n) + std::size_t(i)] * buf_[std::size_t(n) + std::size_t(i)];
    }
    endpoints.level_l.value = n ? std::sqrt(sl / float(n)) : 0.f;
    endpoints.level_r.value = n ? std::sqrt(sr / float(n)) : 0.f;
    endpoints.audio_out.value = AudioBuffer{buf_.data(), n, 2, in.sample_rate};
}
