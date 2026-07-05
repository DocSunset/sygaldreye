#pragma once
#include <Eigen/Geometry>
#include <cstdint>
#include <optional>
#include <vector>

using NodeId = uint32_t;
constexpr NodeId k_null_node = UINT32_MAX;

struct Node {
    Eigen::Affine3f local_transform = Eigen::Affine3f::Identity();
    NodeId          parent          = k_null_node;
};

class SceneGraph {
public:
    NodeId          add_node(NodeId parent = k_null_node);
    void            set_transform(NodeId, Eigen::Affine3f const&);
    Eigen::Matrix4f world_transform(NodeId) const;
    void            invalidate(NodeId);

private:
    std::vector<Node>                          m_nodes;
    mutable std::vector<std::optional<Eigen::Matrix4f>> m_cache;
};
