// Copyright 2026 Travis West
#pragma once
#include <cmath>
#include <memory>
#include <string_view>

#include <Eigen/Core>

#include "eyeballs_node_abi.hpp"
#include "sygaldry_endpoints.hpp"
#include "tree_generator.hpp"
#include "tri_mesh.hpp"

// One tree as a graph node — the forest-route-1 generator (conformability.md).
// `seed` is a CELL input (scalar): wire ONE seed for a single tree, or an
// N×1 seeds Span and the executor LIFTS this node into N instances, each
// generating a DISTINCT tree (cost N generations). lift_key = seed so a
// lifted instance keeps its cached mesh when the seed array reorders.
// Emits the branch geometry as a MeshPtr (the lifted outputs gather into a
// MeshArray for a forest draw).
struct TreeGeneratorNode {
    static consteval std::string_view name() { return "tree_generator"; }
    static consteval std::string_view source_header() {
        return "components/tree_generator/tree_generator_node.hpp";
    }
    static constexpr std::string_view lift_key() { return "seed"; }
    struct endpoints {
        normalled_in<float, fp(0.f), fp(1000.f), fp(42.f)> seed;
        normalled_in<float, fp(1.f), fp(10.f),   fp(4.f)>  height;
        normalled_in<float, fp(0.f), fp(100.f),  fp(8.f)>  spread;  // scatter radius
        out<MeshPtr> mesh;
    } endpoints;
    void operator()(double) {
        uint32_t s = uint32_t(endpoints.seed.get() + 0.5f);
        float    h = endpoints.height.get();
        float    sp = endpoints.spread.get();
        if (cached_ && s == seed_ && h == height_ && sp == spread_) return;
        seed_ = s; height_ = h; spread_ = sp;
        TreeParams tp;
        tp.branch.seed = s;
        tp.trunk_height = h;
        TreeMesh tm = generate_tree(tp);
        // Scatter each tree deterministically from its seed so a lifted
        // forest spreads across the ground (xorshift on the seed → disc).
        uint32_t rng = 0x9e3779b9u + s * 2654435761u;
        auto next = [&rng] {
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            return float(rng) / 4294967296.f;
        };
        float ang = next() * 6.2831853f, dist = std::sqrt(next()) * sp;
        Eigen::Vector3f off{std::cos(ang) * dist, 0.f, std::sin(ang) * dist};
        for (auto& v : tm.branches.vertices) v.position += off;
        cached_ = std::make_shared<const TriMeshData>(std::move(tm.branches));
        endpoints.mesh.value = cached_;
    }

private:
    MeshPtr  cached_;
    uint32_t seed_ = 0xffffffffu;
    float    height_ = -1.f, spread_ = -1.f;
};
