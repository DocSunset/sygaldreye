// Copyright 2026 Travis West
// Emscripten transport for WsLink: the browser's own WebSocket, no thread.
// Outbound messages queue until the socket opens, then flush on each send
// (and on open).
#include "ws_link.hpp"
#include <emscripten/websocket.h>
#include <utility>

namespace {

EM_BOOL on_open(int, const EmscriptenWebSocketOpenEvent* e, void* user) {
    auto* link = static_cast<WsLink*>(user);
    link->set_connected(true);
    for (auto& msg : link->take_outbox())
        emscripten_websocket_send_utf8_text(e->socket, msg.c_str());
    return EM_TRUE;
}

EM_BOOL on_msg(int, const EmscriptenWebSocketMessageEvent* e, void* user) {
    auto* link = static_cast<WsLink*>(user);
    if (e->isText && link->on_message)
        link->on_message(std::string_view(reinterpret_cast<char*>(e->data),
                                          e->numBytes ? e->numBytes - 1 : 0));
    return EM_TRUE;
}

EM_BOOL on_close(int, const EmscriptenWebSocketCloseEvent*, void* user) {
    static_cast<WsLink*>(user)->set_connected(false);
    return EM_TRUE;
}

} // namespace

WsLink::WsLink(std::string url) : url_(std::move(url)) {
    EmscriptenWebSocketCreateAttributes attrs = {url_.c_str(), nullptr, EM_TRUE};
    sock_ = emscripten_websocket_new(&attrs);
    if (sock_ <= 0) return;
    emscripten_websocket_set_onopen_callback(sock_, this, on_open);
    emscripten_websocket_set_onmessage_callback(sock_, this, on_msg);
    emscripten_websocket_set_onclose_callback(sock_, this, on_close);
}

WsLink::~WsLink() {
    if (sock_ > 0) emscripten_websocket_close(sock_, 1000, "bye");
}

void WsLink::send(std::string msg) {
    if (connected_.load() && sock_ > 0) {
        emscripten_websocket_send_utf8_text(sock_, msg.c_str());
    } else {
        std::lock_guard<std::mutex> lock(m_);
        outbox_.push_back(std::move(msg));
    }
}

std::vector<std::string> WsLink::take_outbox() {
    std::lock_guard<std::mutex> lock(m_);
    return std::exchange(outbox_, {});
}

void WsLink::run() {}
