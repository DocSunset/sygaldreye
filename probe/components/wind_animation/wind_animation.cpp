#include "wind_animation.hpp"
#include "tri_mesh.hpp"

void stamp_sway_mask(TriMeshData& mesh, float height_threshold) {
    for (auto& v : mesh.vertices) {
        v.color.w() = (v.position.y() > height_threshold) ? 1.0f : 0.0f;
    }
}
