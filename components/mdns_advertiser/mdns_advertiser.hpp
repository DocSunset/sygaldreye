// Copyright 2025 Travis West
#pragma once
#include <memory>
#include <string_view>

struct MdnsAdvertiser {
    // Start advertising the given service on the given port.
    // Answers mDNS queries from a background thread.
    void start(std::string_view service_name, int port);
    void stop();
    ~MdnsAdvertiser() { stop(); }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
