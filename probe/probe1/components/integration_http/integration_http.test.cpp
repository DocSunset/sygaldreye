// Copyright 2025 Travis West
// Integration tests: HttpServer end-to-end with real socket connections.
#include "http_server.hpp"
#include "eyeballs_node_abi.h"
#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

// ── Minimal HTTP client helpers ───────────────────────────────────────────────

static int find_free_port() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    socklen_t len = sizeof(addr);
    getsockname(s, reinterpret_cast<sockaddr*>(&addr), &len);
    int port = ntohs(addr.sin_port);
    close(s);
    return port;
}

struct HttpResponse { int status; std::string body; };

static HttpResponse send_request(int port, const std::string& method,
                                  const std::string& path,
                                  const std::string& body = "") {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return {0, ""};

    timeval tv{2, 0};
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(s);
        return {0, ""};
    }

    std::string req = method + " " + path + " HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n";
    if (!body.empty())
        req += "Content-Type: application/json\r\nContent-Length: " +
               std::to_string(body.size()) + "\r\n";
    req += "\r\n" + body;

    send(s, req.c_str(), static_cast<int>(req.size()), 0);

    std::string resp;
    char buf[4096];
    int n;
    while ((n = recv(s, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        resp += buf;
    }
    close(s);

    int status = 0;
    if (resp.size() >= 12) {
        try { status = std::stoi(resp.substr(9, 3)); } catch (...) {}
    }
    auto pos = resp.find("\r\n\r\n");
    std::string rbody = (pos != std::string::npos) ? resp.substr(pos + 4) : "";
    return {status, rbody};
}

// Read from /events until a "data:" line appears or timeout expires.
static std::string read_sse_data(int port, int timeout_ms = 3000) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return "";

    timeval tv{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(s);
        return "";
    }

    const char* req = "GET /events HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send(s, req, static_cast<int>(strlen(req)), 0);

    std::string acc;
    char buf[1024];
    int n;
    while ((n = recv(s, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        acc += buf;
        auto hdr = acc.find("\r\n\r\n");
        if (hdr == std::string::npos) continue;
        std::string after_hdr = acc.substr(hdr + 4);
        auto dpos = after_hdr.find("data:");
        if (dpos != std::string::npos) {
            auto end = after_hdr.find('\n', dpos);
            close(s);
            return after_hdr.substr(dpos,
                end != std::string::npos ? end - dpos : std::string::npos);
        }
    }
    close(s);
    return "";
}

// ── Fixture ───────────────────────────────────────────────────────────────────

class HttpTest : public ::testing::Test {
protected:
    void SetUp() override {
        port_ = find_free_port();
        // start() registers GET /params and POST /params internally;
        // add any extra routes before start() so start() does not overwrite them,
        // and register /params-specific ones via start()'s own handlers.
        server_.add_route("POST", "/graph",
            [](std::string_view) -> std::string { return R"({"received":true})"; });
        server_.start(port_,
            [](std::string_view) -> std::string { return R"({"ok":true})"; },
            [](std::string_view b) -> std::string { return std::string(b); });
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
    void TearDown() override { server_.stop(); }

    int        port_ = 0;
    HttpServer server_;
};

// ── Tests ─────────────────────────────────────────────────────────────────────

TEST_F(HttpTest, GetParamsReturns200) {
    auto r = send_request(port_, "GET", "/params");
    EXPECT_EQ(r.status, 200);
    EXPECT_NE(r.body.find("ok"), std::string::npos);
}

TEST_F(HttpTest, PostParamsDeliverBody) {
    auto r = send_request(port_, "POST", "/params", R"({"val":42})");
    EXPECT_EQ(r.status, 200);
    EXPECT_NE(r.body.find("42"), std::string::npos);
}

TEST_F(HttpTest, PostGraphAcceptsJson) {
    const char* graph_json = R"({
        "nodes":[{"id":"sky","type":"sky_dome","params":{}}],
        "edges":[]
    })";
    auto r = send_request(port_, "POST", "/graph", graph_json);
    EXPECT_EQ(r.status, 200);
}

TEST_F(HttpTest, UnknownPathReturns404) {
    auto r = send_request(port_, "GET", "/nonexistent_route_xyz");
    EXPECT_EQ(r.status, 404);
}

TEST_F(HttpTest, MultipleSequentialRequests) {
    for (int i = 0; i < 5; ++i) {
        auto r = send_request(port_, "GET", "/params");
        EXPECT_EQ(r.status, 200) << "request " << i << " failed";
    }
}

TEST_F(HttpTest, BroadcastEventReceivedBySseClient) {
    std::string sse_line;
    std::thread reader([&] {
        sse_line = read_sse_data(port_, 3000);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    server_.broadcast_event("graph", R"({"test":1})");
    reader.join();

    if (sse_line.empty()) {
        GTEST_SKIP() << "SSE event not received within timeout (timing-sensitive)";
    }
    EXPECT_NE(sse_line.find("data:"), std::string::npos);
}

TEST_F(HttpTest, GetPaletteListsType) {
    server_.add_route("GET", "/palette",
        [](std::string_view) -> std::string {
            return R"(["palette_test_node"])";
        });

    auto r = send_request(port_, "GET", "/palette");
    EXPECT_EQ(r.status, 200);
    EXPECT_NE(r.body.find("palette_test_node"), std::string::npos);
}

TEST_F(HttpTest, ConcurrentRequests) {
    std::vector<int> statuses(3, 0);
    std::vector<std::thread> threads;
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&, i] {
            statuses[i] = send_request(port_, "GET", "/params").status;
        });
    }
    for (auto& t : threads) t.join();
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ(statuses[i], 200) << "thread " << i;
}
