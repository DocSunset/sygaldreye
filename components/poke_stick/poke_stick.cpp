// Copyright 2026 Travis West
#include "poke_stick.hpp"

#include <memory>

#include "tri_mesh.hpp"

#include "common_shaders.hpp"

namespace {

// Unit box ([-0.5,0.5]^3) transformed by model, baked into world space.
// Refills m in place (the pose changes every frame; caller touch()es).
void fill_box(TriMeshData& m, const Eigen::Matrix4f& model) {
    m.vertices.clear();
    m.indices.clear();
    const Eigen::Vector3f n[6] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (int f = 0; f < 6; ++f) {
        Eigen::Vector3f u = (f < 2) ? Eigen::Vector3f{0, 1, 0} : Eigen::Vector3f{1, 0, 0};
        Eigen::Vector3f v = n[f].cross(u);
        uint32_t base = uint32_t(m.vertices.size());
        for (int k = 0; k < 4; ++k) {
            float du = (k == 1 || k == 2) ? 1.f : -1.f;
            float dv = (k >= 2) ? 1.f : -1.f;
            Eigen::Vector3f p = (n[f] + u * du + v * dv) * 0.5f;
            Eigen::Vector4f wp = model * p.homogeneous();
            m.vertices.push_back({wp.head<3>(), n[f], {1, 1, 1, 1}});
        }
        m.indices.insert(m.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    }
}

}  // namespace

void PokeStickNode::operator()(double) {
    // Unlit uniform color: uColor flips highlight state with no rebuild.
    if (!shader_)
        shader_ = std::make_shared<ShaderData>(ShaderData{
            common_shaders::kUnlitUniformColorVert, common_shaders::kUnlitUniformColorFrag});

    Eigen::Vector3f pos = endpoints.pos.get();
    Eigen::Quaternionf rot = endpoints.rot.get().normalized();
    float len = endpoints.length.get(), rad = endpoints.radius.get();

    endpoints.tip_pos.value = pos + rot * Eigen::Vector3f(0.f, 0.f, -len);

    // Box centered halfway along the stick, pointing down controller -Z.
    Eigen::Affine3f model = Eigen::Translation3f(pos) * rot *
                            Eigen::Translation3f(0.f, 0.f, -len * 0.5f) *
                            Eigen::Scaling(rad, rad, len);
    if (!data_) data_ = std::make_shared<TriMeshData>();
    fill_box(*data_, model.matrix());
    data_->touch();
    Mesh m;
    m.geometry = data_;
    m.dynamic = true;  // pose baked; refilled every frame
    endpoints.mesh.value = std::move(m);

    bool active = endpoints.active.get() > 0.5f;
    Eigen::Vector4f color =
        active ? Eigen::Vector4f{1.f, 0.8f, 0.2f, 1.f} : Eigen::Vector4f{0.8f, 0.8f, 0.85f, 1.f};
    Surface s;
    s.shader = shader_;
    s.uniforms.push_back({"uColor", color});
    endpoints.surface.value = std::move(s);
}

#ifdef SYGALDREYE_PLUGIN
#include "eyeballs_node_abi.hpp"
EYEBALLS_EXPORT_NODE(PokeStickNode)
#endif
