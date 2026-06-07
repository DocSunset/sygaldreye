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

    std::string  text_copy      = text;
    Eigen::Matrix4f transform_copy = transform;
    outputs.render.value = [this, text_copy, transform_copy](const Eigen::Matrix4f& pv) {
        mesh_.draw(text_copy, pv * transform_copy);
    };
}

std::string to_json(const TextLabelNode& n) {
    char buf[256];
    // Escape text: for now assume no special characters
    std::snprintf(buf, sizeof(buf),
        "{\"text\":\"%s\",\"pos_x\":%g,\"pos_y\":%g,\"pos_z\":%g,\"scale\":%g}",
        n.text.c_str(),
        static_cast<double>(n.inputs.pos_x.value),
        static_cast<double>(n.inputs.pos_y.value),
        static_cast<double>(n.inputs.pos_z.value),
        static_cast<double>(n.inputs.scale.value));
    return buf;
}

void from_json(TextLabelNode& n, std::string_view json) {
    // Parse "text":"..." first
    auto tpos = json.find("\"text\"");
    if (tpos != std::string_view::npos) {
        auto q1 = json.find('"', tpos + 6);  // skip past "text":
        if (q1 != std::string_view::npos) {
            q1 = json.find('"', q1 + 1);  // opening quote of value
            if (q1 != std::string_view::npos) {
                auto q2 = json.find('"', q1 + 1);
                if (q2 != std::string_view::npos)
                    n.text = std::string(json.substr(q1 + 1, q2 - q1 - 1));
            }
        }
    }

    // Parse scalar fields
    auto parse_float = [&](std::string_view key, float& out) {
        auto pos = json.find(key);
        if (pos == std::string_view::npos) return;
        pos = json.find(':', pos + key.size());
        if (pos == std::string_view::npos) return;
        ++pos;
        while (pos < json.size() && json[pos] == ' ') ++pos;
        std::sscanf(json.data() + pos, "%f", &out);
    };

    parse_float("\"pos_x\"", n.inputs.pos_x.value);
    parse_float("\"pos_y\"", n.inputs.pos_y.value);
    parse_float("\"pos_z\"", n.inputs.pos_z.value);
    parse_float("\"scale\"", n.inputs.scale.value);
}
