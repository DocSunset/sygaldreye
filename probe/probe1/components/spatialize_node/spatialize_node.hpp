// Copyright 2026 Travis West
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string_view>
#include <vector>

#include "hrtf_spatializer.hpp"
#include "sygaldry_endpoints.hpp"

// Spatial audio as wiring: mono in, world position + listener pose in
// (vec3/quat — latched across the frame→block boundary), stereo out.
// Sounds attach to scene objects by edging their position into `pos`;
// the head node feeds every spatialize node's listener inputs. AudioScene
// (source registry + mix loop) dissolves into graph topology: sources are
// nodes, the mix is a mix node, the listener is an edge.
//
// KERNEL-EXTRACTION EXCEPTION (kernel_extraction.md): spatialize is not a
// per-sample Lift kernel. It CONSUMES the channel axis (mono in) and
// PRODUCES a new one (stereo out) via per-block HRTF convolution — an
// axis-transforming node, not an element-wise one. The HRTF block math
// stays whole-buffer (Steam Audio v2 is the pending replacement).
struct SpatializeNode {
    static consteval std::string_view name() { return "spatialize"; }
    static consteval std::string_view source_header() {
        return "components/spatialize_node/spatialize_node.hpp";
    }

    // endpoints v6: unconnected audio is structurally silent; pos and the
    // listener pose are normalled (latches write the fallback) with SAFE
    // defaults via endpoint_default (zero vec, identity quat — the
    // uninitialized-Eigen drone class is dead by construction).
    struct endpoints {
        in<AudioBuffer> audio;
        normalled_in<Eigen::Vector3f> pos;
        normalled_in<Eigen::Vector3f> listener_pos;
        normalled_in<Eigen::Quaternionf> listener_rot;
        normalled_in<float, fp(0.f), fp(4.f), fp(1.f)> gain;
        out<AudioBuffer> audio_out;
        out<float> level_l;  // per-ear RMS: the agent's
        out<float> level_r;  // pan evidence
    } endpoints;

    void operator()(double);

private:
    HrtfSpatializer sp_;
    std::vector<float> buf_;
};
