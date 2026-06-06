// Copyright 2025 Travis West
#include "mdns_advertiser.hpp"
#include "mdns_advertiser_impl.h"
#include <atomic>
#include <string>
#include <thread>

struct MdnsAdvertiser::Impl {
    mdns_adv_t*      adv     = nullptr;
    std::atomic_bool running{false};
    std::thread      thread;
};

void MdnsAdvertiser::start(std::string_view service_name, int port) {
    impl_ = std::make_unique<Impl>();
    std::string name(service_name);
    impl_->adv = mdns_adv_create(name.c_str(), port);
    if (!impl_->adv) return;
    impl_->running.store(true);
    impl_->thread = std::thread([this]() {
        while (impl_->running.load()) {
            mdns_adv_poll(impl_->adv, 100);
        }
    });
}

void MdnsAdvertiser::stop() {
    if (!impl_) return;
    impl_->running.store(false);
    if (impl_->thread.joinable()) impl_->thread.join();
    mdns_adv_destroy(impl_->adv);
    impl_->adv = nullptr;
    impl_.reset();
}
