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
    transform(0, 3) = inputs.pos_x.value;
    transform(1, 3) = inputs.pos_y.value;
    transform(2, 3) = inputs.pos_z.value;
    const float s = inputs.scale.value;
    transform(0, 0) = s;
    transform(1, 1) = s;
    transform(2, 2) = s;

    std::string  text_copy      = inputs.label.value.empty() ? "label" : inputs.label.value;
    Eigen::Matrix4f transform_copy = transform;
    outputs.render.value = [this, text_copy, transform_copy](const Eigen::Matrix4f& pv) {
        mesh_.draw(text_copy, pv * transform_copy);
    };
}

