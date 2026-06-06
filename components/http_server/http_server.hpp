// Copyright 2025 Travis West
#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Callback: receives request body (empty for GET), returns response body (JSON)
using HttpHandler = std::function<std::string(std::string_view body)>;

struct RouteKey {
    std::string method;  // uppercase: "GET", "POST", etc.
    std::string path;
    bool operator==(const RouteKey&) const = default;
};

struct RouteKeyHash {
    std::size_t operator()(const RouteKey& k) const noexcept {
        return std::hash<std::string>{}(k.method + k.path);
    }
};

struct HttpServer {
    // Register a handler for method+path before calling start().
    void add_route(std::string method, std::string path, HttpHandler handler);

    // Start serving on port in a background thread.
    // get_handler: called on GET /params — returns JSON
    // post_handler: called on POST /params with body — returns JSON response
    void start(int port, HttpHandler get_handler, HttpHandler post_handler);

    // Push a server-sent event to all connected SSE clients on /events.
    // Thread-safe: may be called from any thread after start().
    void broadcast_event(std::string_view event_type, std::string_view data);

    void stop();
    ~HttpServer() { stop(); }

    HttpServer() = default;
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    // Internal: used by the Mongoose event handler (not for external callers).
    void add_sse_conn(unsigned long id);
    void remove_sse_conn(unsigned long id);
    std::string pop_pending_event();

private:
    std::unordered_map<RouteKey, HttpHandler, RouteKeyHash> routes_;
    std::atomic_bool running_{false};
    std::thread      thread_;

    std::mutex                        sse_mutex_;
    std::unordered_set<unsigned long> sse_conns_;    // active SSE connection ids
    std::vector<std::string>          pending_events_; // queued for broadcast
};
