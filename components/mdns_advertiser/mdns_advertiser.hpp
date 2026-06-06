// Copyright 2025 Travis West
#pragma once
#include <atomic>
#include <string_view>
#include <thread>

struct MdnsAdvertiser {
    // Start advertising the given service on the given port.
    // Answers mDNS queries from a background thread.
    void start(std::string_view service_name, int port);
    void stop();
    ~MdnsAdvertiser() { stop(); }

    MdnsAdvertiser() = default;
    MdnsAdvertiser(const MdnsAdvertiser&) = delete;
    MdnsAdvertiser& operator=(const MdnsAdvertiser&) = delete;

private:
    std::atomic_bool running_{false};
    std::thread      thread_;
};
