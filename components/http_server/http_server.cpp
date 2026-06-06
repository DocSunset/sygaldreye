// Copyright 2025 Travis West
#include "http_server.hpp"
#include "mongoose.h"
#include <cstdio>

using RouteMap = std::unordered_map<RouteKey, HttpHandler, RouteKeyHash>;

static void ev_handler(struct mg_connection* c, int ev, void* ev_data) {
    if (ev != MG_EV_HTTP_MSG) return;
    auto* hm  = static_cast<struct mg_http_message*>(ev_data);
    auto* routes = static_cast<RouteMap*>(c->fn_data);

    std::string method(hm->method.buf, hm->method.len);
    std::string uri(hm->uri.buf, hm->uri.len);
    // uppercase method for lookup
    for (char& ch : method) ch = static_cast<char>(ch & ~0x20);

    auto it = routes->find(RouteKey{method, uri});
    if (it == routes->end()) {
        mg_http_reply(c, 404, "", "Not found\n");
        return;
    }

    std::string body(hm->body.buf, hm->body.len);
    std::string response = it->second(body);
    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                  "%.*s", static_cast<int>(response.size()), response.c_str());
}

void HttpServer::add_route(std::string method, std::string path, HttpHandler handler) {
    routes_[RouteKey{std::move(method), std::move(path)}] = std::move(handler);
}

void HttpServer::start(int port, HttpHandler get_handler, HttpHandler post_handler) {
    add_route("GET",  "/params", std::move(get_handler));
    add_route("POST", "/params", std::move(post_handler));
    running_.store(true);

    thread_ = std::thread([this, port]() {
        struct mg_mgr mgr;
        mg_mgr_init(&mgr);
        char url[64];
        std::snprintf(url, sizeof(url), "http://0.0.0.0:%d", port);
        mg_http_listen(&mgr, url, ev_handler, &routes_);
        while (running_.load()) {
            mg_mgr_poll(&mgr, 100);
        }
        mg_mgr_free(&mgr);
    });
}

void HttpServer::stop() {
    running_.store(false);
    if (thread_.joinable()) thread_.join();
}
