#pragma once
#include <Eigen/Core>
#include <memory>

#include "tri_mesh.hpp"  // TriMeshData, MeshPtr-shaped geometry

using MeshPtr = std::shared_ptr<const TriMeshData>;

// CPU unit-cylinder geometry (radius=1, Z in [-0.5,0.5], centred at origin)
// as a shareable TriMeshData — the render-as-nodes payload. GL lives only in
// render_region; this is pure geometry. Side normals radial, caps axial.
MeshPtr make_cylinder(int slices);

// Model matrix mapping the unit cylinder onto the segment a→b at given radius.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- a,b positionally distinct
Eigen::Matrix4f cylinder_transform(Eigen::Vector3f point_a, Eigen::Vector3f point_b, float radius);
