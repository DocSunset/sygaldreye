// Copyright 2026 Travis West
#include "spatialize_node.hpp"
#include <cmath>

void SpatializeNode::operator()(double) {
    const AudioBuffer& in = inputs.audio.value;
    int n = (in.data && in.frames > 0) ? in.frames : 0;
    buf_.assign(std::size_t(n) * 2, 0.f);

    if (n > 0) {
        // Source direction in listener space; distance drives rolloff/air.
        Eigen::Vector3f rel = inputs.pos.value - inputs.listener_pos.value;
        Eigen::Vector3f dir =
            inputs.listener_rot.value.conjugate() * rel;
        float dist = std::max(0.05f, dir.norm());
        sp_.set_position(dir / dist, dist);
        sp_.process(in.data, buf_.data(), n);
        if (inputs.gain.value != 1.f)
            for (auto& s : buf_) s *= inputs.gain.value;
    }

    float sl = 0.f, sr = 0.f;
    for (int i = 0; i < n; ++i) {
        sl += buf_[std::size_t(i) * 2]     * buf_[std::size_t(i) * 2];
        sr += buf_[std::size_t(i) * 2 + 1] * buf_[std::size_t(i) * 2 + 1];
    }
    outputs.level_l.value = n ? std::sqrt(sl / float(n)) : 0.f;
    outputs.level_r.value = n ? std::sqrt(sr / float(n)) : 0.f;
    outputs.audio.value   = AudioBuffer{buf_.data(), n, 2, in.sample_rate};
}
