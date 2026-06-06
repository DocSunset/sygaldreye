// Copyright 2025 Travis West
#include "mdns_advertiser.hpp"
#include "mdns_advertiser_impl.h"
#include <string>

void MdnsAdvertiser::start(std::string_view service_name, int port) {
    std::string name(service_name);
    running_.store(true);
    thread_ = std::thread([this, name, port]() {
        mdns_adv_t* adv = mdns_adv_create(name.c_str(), port);
        while (running_.load()) {
            mdns_adv_poll(adv, 100);
        }
        mdns_adv_destroy(adv);
    });
}

void MdnsAdvertiser::stop() {
    running_.store(false);
    if (thread_.joinable()) thread_.join();
}
