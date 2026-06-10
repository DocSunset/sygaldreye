// Copyright 2026 Travis West
#pragma once
#include "fly_camera.hpp"

// Applies one frame of FPS-style input to the camera:
// WASD + QE move, hold right mouse button to look. window is a GLFWwindow*.
void apply_fps_input(void* window, FlyCamera& cam, float dt);
