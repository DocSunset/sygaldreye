// Copyright 2026 Travis West
#include "net_nodes.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

namespace {
sockaddr_in loopback(int port) {
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port   = htons(static_cast<uint16_t>(port));
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return a;
}
} // namespace

void UdpSendNode::operator()(double) {
    if (fd_ < 0) fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) return;
    char buf[48];
    int n = std::snprintf(buf, sizeof(buf), "%d %g",
                          int(inputs.channel.value), double(inputs.in.value));
    auto addr = loopback(int(inputs.port.value));
    sendto(fd_, buf, size_t(n), 0,
           reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
}

UdpSendNode::~UdpSendNode() { if (fd_ >= 0) close(fd_); }

void UdpRecvNode::operator()(double) {
    int want = int(inputs.port.value);
    if (fd_ >= 0 && bound_port_ != want) { close(fd_); fd_ = -1; }
    if (fd_ < 0) {
        fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd_ < 0) return;
        int one = 1;
        setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        fcntl(fd_, F_SETFL, O_NONBLOCK);
        auto addr = loopback(want);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            close(fd_); fd_ = -1; return;
        }
        bound_port_ = want;
    }
    char buf[64];
    ssize_t n;
    while ((n = recv(fd_, buf, sizeof(buf) - 1, 0)) > 0) {  // drain; last wins
        buf[n] = '\0';
        int ch; double v;
        if (std::sscanf(buf, "%d %lf", &ch, &v) == 2 &&
            ch == int(inputs.channel.value))
            outputs.out.value = float(v);
    }
}

UdpRecvNode::~UdpRecvNode() { if (fd_ >= 0) close(fd_); }
