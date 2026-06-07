// Copyright 2025 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>

template<typename Derived>
class VisualNode {
public:
    void operator()(double time_s) {
        auto& d = static_cast<Derived&>(*this);
        d.sync_params();
        d.update(static_cast<float>(time_s));
        d.outputs.render.value = [pd = &d](const Eigen::Matrix4f& vp) { pd->draw(vp); };
    }
};
