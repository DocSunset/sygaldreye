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
    struct inputs {
        slider<"in",      "", float, fp(-1e6f),  fp(1e6f),   fp(0.f)>    in;
        slider<"channel", "", float, fp(0.f),    fp(127.f),  fp(0.f)>    channel;
        slider<"port",    "", float, fp(1024.f), fp(65535.f),fp(9100.f)> port;
        ::text<"host"> host;  // empty = 127.0.0.1
    } inputs;
    struct outputs {} outputs;
    void operator()(double);
    ~UdpSendNode();
private:
    int fd_ = -1;
    std::string resolved_host_;
    struct in_addr resolved_addr_ {};
};

struct UdpRecvNode {
    static consteval std::string_view name() { return "udp_recv"; }
    struct inputs {
        slider<"channel", "", float, fp(0.f),    fp(127.f),  fp(0.f)>    channel;
        slider<"port",    "", float, fp(1024.f), fp(65535.f),fp(9100.f)> port;
    } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double);
    ~UdpRecvNode();
private:
    int fd_ = -1;
    int bound_port_ = -1;
};
