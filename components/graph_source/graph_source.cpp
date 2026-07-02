// Copyright 2026 Travis West
#include "graph_source.hpp"

// Key hash + default grid are editor_layout's (one definition: hit-tests must
// agree with lift keys — drift here silently splits card identity).
using editor_layout::default_card_pos;
using editor_layout::id_key;

void GraphSourceNode::operator()(double) {
    if (!graph_) {
        endpoints.keys.value = Span{nullptr, 0, 1};
        endpoints.positions.value = Span{nullptr, 0, 3};
        endpoints.count.value = 0.f;
        return;
    }
    int n = int(graph_->nodes.size());
    keys_.assign(std::size_t(n), 0.f);
    pos_.assign(std::size_t(n) * 3, 0.f);
    for (int i = 0; i < n; ++i) {
        const std::string& id = graph_->nodes[std::size_t(i)].id;
        keys_[std::size_t(i)] = id_key(id);
        Eigen::Vector3f p = default_card_pos(i);
        if (pos_ovr_)
            if (auto ov = pos_ovr_->find(id); ov != pos_ovr_->end()) p = ov->second;
        pos_[std::size_t(i) * 3 + 0] = p.x();
        pos_[std::size_t(i) * 3 + 1] = p.y();
        pos_[std::size_t(i) * 3 + 2] = p.z();
    }
    endpoints.keys.value = Span{keys_.data(), n, 1};
    endpoints.positions.value = Span{pos_.data(), n, 3};
    endpoints.count.value = float(n);
}
