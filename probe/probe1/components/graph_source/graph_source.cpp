// Copyright 2026 Travis West
#include "graph_source.hpp"

namespace {
// Stable id hash → the key column. Identity must follow the node, not its
// slot, so a card keeps its lifted state across reorders and live edits.
float id_key(const std::string& id) {
    std::uint32_t h = 2166136261u;  // FNV-1a
    for (unsigned char c : id) {
        h ^= c;
        h *= 16777619u;
    }
    return float(h & 0xffffffu);  // 24 bits — exact in a float
}

// The default grid layout (mirrors the old VrEditor card placement): a
// reachable band, 8 per row, 4 rows, then a fresh block to the right.
Eigen::Vector3f default_pos(int i) {
    return {
        float(i % 8) * 0.45f - 1.6f + float(i / 32) * 4.0f,
        1.85f - float((i / 8) % 4) * 0.5f,
        -0.5f};
}
}  // namespace

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
        Eigen::Vector3f p = default_pos(i);
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
