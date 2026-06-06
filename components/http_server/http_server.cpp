// Copyright 2025 Travis West
#include "http_server.hpp"
#include "mongoose.h"
#include <atomic>
#include <thread>

struct HandlerCtx {
    HttpHandler get_handler;
    HttpHandler post_handler;
};

static void ev_handler(struct mg_connection* c, int ev, void* ev_data) {
    if (ev != MG_EV_HTTP_MSG) return;
    auto* hm  = static_cast<struct mg_http_message*>(ev_data);
    auto* ctx = static_cast<HandlerCtx*>(c->fn_data);

    if (mg_strcmp(hm->uri, mg_str("/params")) != 0) {
        mg_http_reply(c, 404, "", "Not found\n");
        return;
    }

    std::string body(hm->body.buf, hm->body.len);
    std::string response;
    if (mg_strcasecmp(hm->method, mg_str("GET")) == 0) {
        response = ctx->get_handler("");
    } else if (mg_strcasecmp(hm->method, mg_str("POST")) == 0) {
        response = ctx->post_handler(body);
    } else {
        mg_http_reply(c, 405, "", "Method not allowed\n");
        return;
    }
    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                  "%.*s", static_cast<int>(response.size()), response.c_str());
}

struct HttpServer::Impl {
    HandlerCtx       ctx;
    std::atomic_bool running{false};
    std::thread      thread;
};

void HttpServer::start(int port, HttpHandler get_handler, HttpHandler post_handler) {
    impl_ = std::make_unique<Impl>();
    impl_->ctx.get_handler  = std::move(get_handler);
    impl_->ctx.post_handler = std::move(post_handler);
    impl_->running.store(true);

    impl_->thread = std::thread([this, port]() {
        struct mg_mgr mgr;
        mg_mgr_init(&mgr);
        char url[64];
        std::snprintf(url, sizeof(url), "http://0.0.0.0:%d", port);
        mg_http_listen(&mgr, url, ev_handler, &impl_->ctx);
        while (impl_->running.load()) {
            mg_mgr_poll(&mgr, 100);
        }
        mg_mgr_free(&mgr);
    });
}

void HttpServer::stop() {
    if (!impl_) return;
    impl_->running.store(false);
    if (impl_->thread.joinable()) impl_->thread.join();
    impl_.reset();
}
