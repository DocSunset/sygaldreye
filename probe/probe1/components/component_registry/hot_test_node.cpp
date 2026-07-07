// Copyright 2026 Travis West
// Test fixture plugin: compiled twice (HOT_TEST_VERSION 1 and 2) to
// exercise hot-reload — same type name, different code and behavior.
#include "eyeballs_node_abi.hpp"

struct HotTestNode {
    static consteval std::string_view name() { return "hot_test"; }
    struct endpoints {
        normalled_in<float, fp(0.f), fp(10.f), fp(1.f)> gain;
        out<float> v;
    } endpoints;
    void operator()(double) {
        endpoints.v.value = endpoints.gain.get() * float(HOT_TEST_VERSION);
    }
};

EYEBALLS_EXPORT_NODE(HotTestNode)
