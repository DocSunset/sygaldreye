#include "vr_math.hpp"
#include <Eigen/Geometry>
#include <cassert>
#include <cmath>

static Eigen::Matrix3f quat_to_rot(const XrQuaternionf& q) {
    Eigen::Quaternionf eq(q.w, q.x, q.y, q.z);
    return eq.normalized().toRotationMatrix();
}

Eigen::Matrix4f projection(const XrFovf& fov, float near_z, float far_z) {
    assert(near_z > 0.0F && far_z > near_z);
    float l = std::tan(fov.angleLeft)  * near_z;
    float r = std::tan(fov.angleRight) * near_z;
    float b = std::tan(fov.angleDown)  * near_z;
    float t = std::tan(fov.angleUp)    * near_z;
    assert(r != l && t != b);

    Eigen::Matrix4f m = Eigen::Matrix4f::Zero();
    m(0,0) = 2.f * near_z / (r - l);
    m(0,2) = (r + l) / (r - l);
    m(1,1) = 2.f * near_z / (t - b);
    m(1,2) = (t + b) / (t - b);
    m(2,2) = -(far_z + near_z) / (far_z - near_z);
    m(2,3) = -2.f * far_z * near_z / (far_z - near_z);
    m(3,2) = -1.f;
    return m;
}

Eigen::Matrix4f view(const XrPosef& pose) {
    Eigen::Matrix3f R = quat_to_rot(pose.orientation);
    Eigen::Vector3f t(pose.position.x, pose.position.y, pose.position.z);

    Eigen::Matrix3f Rt = R.transpose();
    Eigen::Vector3f eye = -(Rt * t);

    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
    m.topLeftCorner<3,3>() = Rt;
    m.topRightCorner<3,1>() = eye;
    return m;
}

Eigen::Matrix4f pose_to_world(const XrPosef& pose) {
    Eigen::Matrix3f R = quat_to_rot(pose.orientation);
    Eigen::Vector3f t(pose.position.x, pose.position.y, pose.position.z);

    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
    m.topLeftCorner<3,3>() = R;
    m.topRightCorner<3,1>() = t;
    return m;
}
