// Copyright 2026 Travis West
#pragma once
struct HostApp;

// Applies one frame of FPS-style input to the graph's "camera" node:
// WASD + QE move, hold right mouse button to look. window is a GLFWwindow*.
// Reads camera state from the graph and writes back via queue_param, so
// human input and HTTP control share one path.
void apply_fps_input(void* window, HostApp& app, float dt);
