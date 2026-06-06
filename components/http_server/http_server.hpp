// Copyright 2025 Travis West
#pragma once
#include <atomic>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

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

    void stop();
    ~HttpServer() { stop(); }

    HttpServer() = default;
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

private:
    std::unordered_map<RouteKey, HttpHandler, RouteKeyHash> routes_;
    std::atomic_bool running_{false};
    std::thread      thread_;
};
