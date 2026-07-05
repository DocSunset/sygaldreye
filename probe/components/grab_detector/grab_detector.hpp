#pragma once
#include <Eigen/Core>
#include <span>

struct GrabTarget;

struct HandState {
    Eigen::Vector3f position     = Eigen::Vector3f::Zero();
    bool            valid        = false;
    bool            grip_pressed = false;
};

void update_grabs(std::span<const HandState> hands,
                  std::span<GrabTarget>      targets);
