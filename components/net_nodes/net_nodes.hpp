// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <netinet/in.h>
#include <string>
#include <string_view>

// First network bridge, graph-native: a value leaves one instance's graph
// through udp_send and enters another's through udp_recv. Channel numbers
// pair them. Loopback by default; cross-device addressing comes with the
// full bridge design (needs string params).

struct UdpSendNode {
    static consteval std::string_view name() { return "udp_send"; }
    static constexpr int lift_kind() { return lift::resource_holder; }
    struct endpoints {
        normalled_in<float, fp(-1e6f),  fp(1e6f),   fp(0.f)>    in;
        normalled_in<float, fp(0.f),    fp(127.f),  fp(0.f)>    channel;
        normalled_in<float, fp(1024.f), fp(65535.f),fp(9100.f)> port;
        normalled_in<std::string> host;  // empty = 127.0.0.1
    } endpoints;
    void operator()(double);
    ~UdpSendNode();
private:
    int fd_ = -1;
    std::string resolved_host_;
    struct in_addr resolved_addr_ {};
};

struct UdpRecvNode {
    static consteval std::string_view name() { return "udp_recv"; }
    static constexpr int lift_kind() { return lift::resource_holder; }
    struct endpoints {
        normalled_in<float, fp(0.f),    fp(127.f),  fp(0.f)>    channel;
        normalled_in<float, fp(1024.f), fp(65535.f),fp(9100.f)> port;
        ::out<float> out;
    } endpoints;
    void operator()(double);
    ~UdpRecvNode();
private:
    int fd_ = -1;
    int bound_port_ = -1;
};
