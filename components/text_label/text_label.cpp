// Copyright 2025 Travis West
#include "text_label.hpp"
#include <charconv>
#include <cstdio>

void TextLabelNode::operator()(double /*time_s*/) {
    if (!initialized_) {
        mesh_.init();
        initialized_ = true;
    }

    Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
    transform(0, 3) = endpoints.pos_x.get();
    transform(1, 3) = endpoints.pos_y.get();
    transform(2, 3) = endpoints.pos_z.get();
    const float s = endpoints.scale.get();
    transform(0, 0) = s;
    transform(1, 1) = s;
    transform(2, 2) = s;

    std::string text = endpoints.text.get();
    if (text.empty()) text = "label";
    Eigen::Matrix4f transform_copy = transform;
    endpoints.render.value = [this, text, transform_copy](const Eigen::Matrix4f& pv) {
        mesh_.draw(text, pv * transform_copy);
    };
}
