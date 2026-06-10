#include "scene_graph.hpp"

NodeId SceneGraph::add_node(NodeId parent) {
    NodeId id = static_cast<NodeId>(m_nodes.size());
    m_nodes.push_back({Eigen::Affine3f::Identity(), parent});
    m_cache.push_back(std::nullopt);
    return id;
}

void SceneGraph::set_transform(NodeId id, Eigen::Affine3f const& t) {
    m_nodes[id].local_transform = t;
    invalidate(id);
}

Eigen::Matrix4f SceneGraph::world_transform(NodeId id) const {
    if (m_cache[id].has_value())
        return *m_cache[id];

    Eigen::Matrix4f result;
    NodeId parent = m_nodes[id].parent;
    if (parent == k_null_node)
        result = m_nodes[id].local_transform.matrix();
    else
        result = world_transform(parent) * m_nodes[id].local_transform.matrix();

    m_cache[id] = result;
    return result;
}

void SceneGraph::invalidate(NodeId id) {
    m_cache[id] = std::nullopt;
    for (NodeId i = 0; i < static_cast<NodeId>(m_nodes.size()); ++i) {
        if (m_nodes[i].parent == id)
            invalidate(i);
    }
}
