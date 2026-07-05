#pragma once
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <openxr/openxr.h>
#include <Eigen/Core>

Eigen::Matrix4f projection(const XrFovf& fov, float near_z, float far_z);
Eigen::Matrix4f view(const XrPosef& pose);
Eigen::Matrix4f pose_to_world(const XrPosef& pose);
