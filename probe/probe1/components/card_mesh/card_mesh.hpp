// Copyright 2026 Travis West
#pragma once
#include <Eigen/Core>
#include <memory>
#include <string_view>

#include "render_payloads.hpp"
#include "sygaldry_endpoints.hpp"
#include "tri_mesh.hpp"

// One editor card's geometry as a graph node (vr_editor_decomposition.md S4).
// `position` is a vec3 CELL: wire ONE for a single card, or graph_source's
// N×3 card-positions Span and the executor LIFTS this into N cards — keyed by
// id (the card subgraph's lift_key) so a dragged card keeps its identity.
// Emits a flat quad facing +z at `position` as a MeshPtr; the lifted outputs
// gather into a MeshArray the editor's card_draw enqueues. Geometry only —
// appearance is the Surface from vertex_color_mesh downstream.
struct CardMeshNode {
    static consteval std::string_view name() { return "card_mesh"; }
    static consteval std::string_view source_header() {
        return "components/card_mesh/card_mesh.hpp";
    }
    struct endpoints {
        ::in<float> id;  // keyed-lift key
        ::in<Eigen::Vector3f> position;
        normalled_in<float, fp(0.01f), fp(1.f), fp(0.4f)> width;
        normalled_in<float, fp(0.01f), fp(1.f), fp(0.1f)> height;
        out<MeshPtr> mesh;
    } endpoints;

    void operator()(double) {
        Eigen::Vector3f p = endpoints.position.get();
        float w = endpoints.width.get() * 0.5f, h = endpoints.height.get() * 0.5f;
        if (cached_ && p == pos_ && w == hw_ && h == hh_) return;
        pos_ = p;
        hw_ = w;
        hh_ = h;
        TriMeshData m;
        const Eigen::Vector4f col{0.10f, 0.14f, 0.20f, 0.92f};
        const Eigen::Vector3f n{0.f, 0.f, 1.f};
        const float dx[4] = {-w, w, w, -w};
        const float dy[4] = {-h, -h, h, h};
        for (int i = 0; i < 4; ++i)
            m.vertices.push_back({{p.x() + dx[i], p.y() + dy[i], p.z()}, n, col});
        m.indices = {0, 1, 2, 0, 2, 3};
        cached_ = std::make_shared<const TriMeshData>(std::move(m));
        endpoints.mesh.value = cached_;
    }

private:
    MeshPtr cached_;
    Eigen::Vector3f pos_{1e9f, 0, 0};
    float hw_ = -1.f, hh_ = -1.f;
};
