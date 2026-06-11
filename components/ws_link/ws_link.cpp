// Copyright 2026 Travis West
#include "ws_link.hpp"
#include "mongoose.h"
#include <utility>

namespace {
void ev_handler(struct mg_connection* c, int ev, void* ev_data) {
    auto* link = static_cast<WsLink*>(c->fn_data);
    if (ev == MG_EV_WS_OPEN) {
        link->set_connected(true);
    } else if (ev == MG_EV_WS_MSG) {
        auto* wm = static_cast<struct mg_ws_message*>(ev_data);
        if (link->on_message)
            link->on_message(std::string_view(wm->data.buf, wm->data.len));
    } else if (ev == MG_EV_CLOSE || ev == MG_EV_ERROR) {
        link->set_connected(false);
    }
}
} // namespace

WsLink::WsLink(std::string url)
    : url_(std::move(url)), thread_([this] { run(); }) {}

WsLink::~WsLink() {
    running_.store(false);
    if (thread_.joinable()) thread_.join();
}

void WsLink::send(std::string msg) {
    std::lock_guard<std::mutex> lock(m_);
    outbox_.push_back(std::move(msg));
}

std::vector<std::string> WsLink::take_outbox() {
    std::lock_guard<std::mutex> lock(m_);
    return std::exchange(outbox_, {});
}

void WsLink::run() {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    struct mg_connection* conn = nullptr;
    int backoff_polls = 0;
    while (running_.load()) {
        if (!conn || !connected_.load()) {
            if (backoff_polls > 0) {
                --backoff_polls;
            } else {
                conn = mg_ws_connect(&mgr, url_.c_str(), ev_handler, this, nullptr);
                backoff_polls = 50;  // ~0.5 s between attempts
            }
        }
        if (conn && connected_.load()) {
            for (auto& msg : take_outbox())
                mg_ws_send(conn, msg.data(), msg.size(), WEBSOCKET_OP_TEXT);
        }
        mg_mgr_poll(&mgr, 10);
    }
    mg_mgr_free(&mgr);
}
