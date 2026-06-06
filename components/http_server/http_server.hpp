// Copyright 2025 Travis West
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <string_view>

// Callback: receives request body (empty for GET), returns response body (JSON)
using HttpHandler = std::function<std::string(std::string_view body)>;

struct HttpServer {
    // Start serving on port in a background thread.
    // get_handler: called on GET /params — returns JSON
    // post_handler: called on POST /params with body — returns JSON response
    void start(int port, HttpHandler get_handler, HttpHandler post_handler);
    void stop();
    ~HttpServer() { stop(); }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
