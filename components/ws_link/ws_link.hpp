// Copyright 2026 Travis West
#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// WebSocket client connection to another peer — the consumer half of the
// net mapping transport. Native: owns a mongoose poll thread; send()
// queues from any thread; on_message fires on the poll thread; reconnects
// automatically. Browser (Emscripten): single-threaded, backed by the
// browser's WebSocket; callbacks fire on the main loop.
class WsLink {
public:
    explicit WsLink(std::string url);
    ~WsLink();
    WsLink(const WsLink&)            = delete;
    WsLink& operator=(const WsLink&) = delete;

    void send(std::string msg);
    bool connected() const { return connected_.load(); }
    const std::string& url() const { return url_; }

    // Set before any traffic is expected; poll-thread context (native) or
    // main loop (browser).
    std::function<void(std::string_view msg)> on_message;

    // Internal (transport callbacks).
    std::vector<std::string> take_outbox();
    void set_connected(bool c) { connected_.store(c); }

private:
    void run();

    std::string              url_;
    std::atomic_bool         running_{true};
    std::atomic_bool         connected_{false};
    std::mutex               m_;
    std::vector<std::string> outbox_;
#ifdef __EMSCRIPTEN__
    int                      sock_ = 0;
#else
    std::thread              thread_;
#endif
};
