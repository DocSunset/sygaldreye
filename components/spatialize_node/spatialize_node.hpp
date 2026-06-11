// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include "hrtf_spatializer.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string_view>
#include <vector>

// Spatial audio as wiring: mono in, world position + listener pose in
// (vec3/quat — latched across the frame→block boundary), stereo out.
// Sounds attach to scene objects by edging their position into `pos`;
// the head node feeds every spatialize node's listener inputs. AudioScene
// (source registry + mix loop) dissolves into graph topology: sources are
// nodes, the mix is a mix node, the listener is an edge.
struct SpatializeNode {
    static consteval std::string_view name() { return "spatialize"; }
    static consteval std::string_view source_header() { return "components/spatialize_node/spatialize_node.hpp"; }

    struct inputs {
        port<"audio",        AudioBuffer>        audio;
        port<"pos",          Eigen::Vector3f>    pos;
        port<"listener_pos", Eigen::Vector3f>    listener_pos;
        port<"listener_rot", Eigen::Quaternionf> listener_rot;
        slider<"gain", "", float, fp(0.f), fp(4.f), fp(1.f)> gain;
    } inputs;

    struct outputs {
        port<"audio",   AudioBuffer> audio;   // interleaved stereo
        port<"level_l", float>       level_l; // per-ear RMS: the agent's
        port<"level_r", float>       level_r; // pan evidence
    } outputs;

    SpatializeNode() { inputs.listener_rot.value.setIdentity(); }
    void operator()(double);

private:
    HrtfSpatializer    sp_;
    std::vector<float> buf_;
};
