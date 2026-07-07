// Copyright 2025 Travis West
#include "http_server.hpp"
#include "mongoose.h"
#include <cstdio>
#include <utility>

using RouteMap = std::unordered_map<RouteKey, HttpHandler, RouteKeyHash>;

// Two-pointer context passed as fn_data to Mongoose connections.
struct ServerCtx {
    HttpServer* server;
    RouteMap*   routes;
};

static void ev_handler(struct mg_connection* c, int ev, void* ev_data) {
    auto* ctx = static_cast<ServerCtx*>(c->fn_data);

    if (ev == MG_EV_CLOSE) {
        ctx->server->remove_sse_conn(c->id);
        return;
    }
    if (ev == MG_EV_WS_MSG) {
        auto* wm = static_cast<struct mg_ws_message*>(ev_data);
        if (ctx->server->ws_handler) {
            std::string reply = ctx->server->ws_handler(
                c->id, std::string_view(wm->data.buf, wm->data.len));
            if (!reply.empty())
                mg_ws_send(c, reply.data(), reply.size(), WEBSOCKET_OP_TEXT);
        }
        return;
    }
    if (ev != MG_EV_HTTP_MSG) return;

    auto* hm = static_cast<struct mg_http_message*>(ev_data);
    std::string method(hm->method.buf, hm->method.len);
    std::string uri(hm->uri.buf, hm->uri.len);
    for (char& ch : method) ch = static_cast<char>(ch & ~0x20);

    if (method == "GET" && uri == "/ws") {
        mg_ws_upgrade(c, hm, nullptr);
        return;
    }
    if (method == "GET" && uri == "/events") {
        mg_printf(c,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/event-stream\r\n"
            "Cache-Control: no-cache\r\n"
            "Connection: keep-alive\r\n\r\n");
        c->pfn_data = reinterpret_cast<void*>(1);  // mark as SSE client
        ctx->server->add_sse_conn(c->id);
        return;
    }

    auto it = ctx->routes->find(RouteKey{method, uri});
    if (it == ctx->routes->end()) {
        mg_http_reply(c, 404, "", "Not found\n");
        return;
    }
    HttpServer::last_query_.assign(hm->query.buf, hm->query.len);
    std::string body(hm->body.buf, hm->body.len);
    std::string response = it->second(body);
    // Binary-safe: %.*s would truncate at the first NUL (PNG bodies).
    mg_printf(c,
              "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n"
              "Content-Length: %d\r\nConnection: close\r\n\r\n",
              static_cast<int>(response.size()));
    mg_send(c, response.data(), response.size());
}

void HttpServer::add_route(std::string method, std::string path, HttpHandler handler) {
    routes_[RouteKey{std::move(method), std::move(path)}] = std::move(handler);
}

void HttpServer::add_sse_conn(unsigned long id) {
    std::lock_guard<std::mutex> lk(sse_mutex_);
    sse_conns_.insert(id);
}

void HttpServer::remove_sse_conn(unsigned long id) {
    std::lock_guard<std::mutex> lk(sse_mutex_);
    sse_conns_.erase(id);
}

std::string HttpServer::pop_pending_event() {
    std::lock_guard<std::mutex> lk(sse_mutex_);
    if (pending_events_.empty()) return {};
    std::string msg = std::move(pending_events_.front());
    pending_events_.erase(pending_events_.begin());
    return msg;
}

void HttpServer::ws_send(unsigned long conn_id, std::string msg) {
    std::lock_guard<std::mutex> lk(ws_mutex_);
    ws_outbox_.emplace_back(conn_id, std::move(msg));
}

std::vector<std::pair<unsigned long, std::string>> HttpServer::take_ws_outbox() {
    std::lock_guard<std::mutex> lk(ws_mutex_);
    return std::exchange(ws_outbox_, {});
}

void HttpServer::broadcast_event(std::string_view event_type, std::string_view data) {
    std::string frame = "event: ";
    frame += event_type;
    frame += "\ndata: ";
    frame += data;
    frame += "\n\n";
    std::lock_guard<std::mutex> lk(sse_mutex_);
    pending_events_.push_back(std::move(frame));
}

void HttpServer::start(int port, HttpHandler get_handler, HttpHandler post_handler) {
    add_route("GET",  "/params", std::move(get_handler));
    add_route("POST", "/params", std::move(post_handler));
    running_.store(true);

    thread_ = std::thread([this, port]() {
        struct mg_mgr mgr;
        mg_mgr_init(&mgr);

        ServerCtx ctx{this, &routes_};
        char url[64];
        std::snprintf(url, sizeof(url), "http://0.0.0.0:%d", port);
        mg_http_listen(&mgr, url, ev_handler, &ctx);

        while (running_.load()) {
            mg_mgr_poll(&mgr, 10);
            // Drain pending SSE events and send to all marked SSE clients.
            for (;;) {
                std::string msg = pop_pending_event();
                if (msg.empty()) break;
                for (struct mg_connection* nc = mgr.conns; nc; nc = nc->next) {
                    if (nc->fn_data == &ctx && nc->pfn_data)
                        mg_printf(nc, "%.*s", static_cast<int>(msg.size()), msg.c_str());
                }
            }
            // Drain queued outbound WebSocket frames.
            for (auto& [id, msg] : take_ws_outbox()) {
                for (struct mg_connection* nc = mgr.conns; nc; nc = nc->next) {
                    if (nc->id == id && nc->is_websocket) {
                        mg_ws_send(nc, msg.data(), msg.size(), WEBSOCKET_OP_TEXT);
                        break;
                    }
                }
            }
        }
        mg_mgr_free(&mgr);
    });
}

void HttpServer::stop() {
    running_.store(false);
    if (thread_.joinable()) thread_.join();
}
