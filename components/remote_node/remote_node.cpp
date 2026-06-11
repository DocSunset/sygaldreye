// Copyright 2026 Travis West
#include "remote_node.hpp"
#include "port_schema_reader.hpp"
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <utility>

namespace {

std::string_view find_string(std::string_view json, std::string_view key) {
    std::string needle = "\"" + std::string(key) + "\":\"";
    auto p = json.find(needle);
    if (p == std::string_view::npos) return {};
    p += needle.size();
    auto e = json.find('"', p);
    return (e == std::string_view::npos) ? std::string_view{} : json.substr(p, e - p);
}

// Value of `key` in a compact JSON object: number, array, or object.
std::string_view find_value(std::string_view json, std::string_view key) {
    std::string needle = "\"" + std::string(key) + "\":";
    auto p = json.find(needle);
    if (p == std::string_view::npos) return {};
    p += needle.size();
    std::size_t i = p;
    int depth = 0;
    bool in_str = false;
    for (; i < json.size(); ++i) {
        char ch = json[i];
        if (in_str) { if (ch == '"' && json[i-1] != '\\') in_str = false; continue; }
        if (ch == '"') in_str = true;
        else if (ch == '{' || ch == '[') ++depth;
        else if (ch == '}' || ch == ']') { if (depth == 0) break; --depth; }
        else if (ch == ',' && depth == 0) break;
    }
    return json.substr(p, i - p);
}

std::array<float, 4> parse_floats(std::string_view s) {
    std::array<float, 4> out{};
    std::size_t i = 0, n = 0;
    while (i < s.size() && n < 4) {
        while (i < s.size() && (s[i] == '[' || s[i] == ',' || s[i] == ' ')) ++i;
        if (i >= s.size() || s[i] == ']') break;
        out[n++] = std::strtof(std::string(s.substr(i)).c_str(), nullptr);
        while (i < s.size() && s[i] != ',' && s[i] != ']') ++i;
    }
    return out;
}

std::string next_uid(const std::string& type) {
    static std::atomic<int> counter{0};
    char buf[96];
    std::snprintf(buf, sizeof(buf), "r%d_%s", ++counter, type.c_str());
    return buf;
}

} // namespace

// ── RemotePeer ───────────────────────────────────────────────────────────────

RemotePeer::RemotePeer(std::string ws_url, std::string alias)
    : link_(std::move(ws_url)), alias_(std::move(alias)) {
    link_.on_message = [this](std::string_view m) { handle(m); };
}

void RemotePeer::handle(std::string_view msg) {
    auto op = find_string(msg, "op");
    if (op == "out") {
        std::string id{find_string(msg, "id")};
        auto vals = find_value(msg, "values");
        std::lock_guard<std::mutex> lock(m_);
        latest_[id] = std::string(vals);
    } else if (op == "types") {
        auto arr = find_value(msg, "types");
        // split top-level objects of the array
        std::lock_guard<std::mutex> lock(m_);
        types_.clear();
        int depth = 0;
        std::size_t start = 0;
        bool in_str = false;
        for (std::size_t i = 0; i < arr.size(); ++i) {
            char ch = arr[i];
            if (in_str) { if (ch == '"' && arr[i-1] != '\\') in_str = false; continue; }
            if (ch == '"') in_str = true;
            else if (ch == '{') { if (++depth == 1) start = i; }
            else if (ch == '}') {
                if (--depth == 0) {
                    auto obj = arr.substr(start, i - start + 1);
                    TypeInfo t;
                    t.type   = std::string(find_string(obj, "type"));
                    t.schema = std::string(find_value(obj, "schema"));
                    if (!t.type.empty()) types_.push_back(std::move(t));
                }
            }
        }
        types_received_.store(true);
    }
}

bool RemotePeer::fetch_types(int timeout_ms) {
    types_received_.store(false);
    for (int waited = 0; waited < timeout_ms; waited += 50) {
        if (link_.connected()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    link_.send(R"({"op":"types"})");
    for (int waited = 0; waited < timeout_ms; waited += 50) {
        if (types_received_.load()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;
}

void RemotePeer::spawn(const std::string& uid, const std::string& remote_type,
                       const std::string& params_json) {
    link_.send(R"({"op":"spawn","id":")" + uid + R"(","type":")" + remote_type +
               R"(","params":)" + (params_json.empty() ? "{}" : params_json) + "}");
}

void RemotePeer::set_params(const std::string& uid, const std::string& params_json) {
    link_.send(R"({"op":"set","id":")" + uid + R"(","params":)" + params_json + "}");
}

void RemotePeer::despawn(const std::string& uid) {
    link_.send(R"({"op":"despawn","id":")" + uid + "\"}");
    std::lock_guard<std::mutex> lock(m_);
    latest_.erase(uid);
}

std::string RemotePeer::latest(const std::string& uid) {
    std::lock_guard<std::mutex> lock(m_);
    auto it = latest_.find(uid);
    return it == latest_.end() ? std::string{} : it->second;
}

// ── RemoteNode ───────────────────────────────────────────────────────────────

RemoteNode::RemoteNode(RemotePeer* peer, std::string remote_type, std::string schema)
    : peer_(peer), remote_type_(std::move(remote_type)), schema_(std::move(schema)),
      uid_(next_uid(remote_type_)) {
    peer_->spawn(uid_, remote_type_, "{}");
}

RemoteNode::~RemoteNode() { peer_->despawn(uid_); }

void RemoteNode::set_in(const std::string& port, std::string value_json) {
    auto it = pending_.find(port);
    if (it != pending_.end() && it->second == value_json) return;
    pending_[port] = std::move(value_json);
    dirty_ = true;
}

void RemoteNode::process(double) {
    if (!dirty_) return;
    dirty_ = false;
    std::string params = "{";
    bool first = true;
    for (auto& [k, v] : pending_) {
        if (!first) params += ',';
        first = false;
        params += '"'; params += k; params += "\":"; params += v;
    }
    params += '}';
    peer_->set_params(uid_, params);
}

void RemoteNode::push_outputs(EyeballsOutputCtx* ctx) {
    std::string vals = peer_->latest(uid_);
    if (vals.empty()) return;
    PortSchema schema = parse_port_schema(schema_.c_str());
    for (const auto& p : schema.outputs) {
        auto v = find_value(vals, p.name);
        if (v.empty()) continue;
        const char* nm = p.name.c_str();
        if (p.kind == "scalar" || p.kind == "bang" || p.kind == "bool") {
            ctx->emit_scalar(ctx->store, ctx->node_id, nm,
                             std::strtod(std::string(v).c_str(), nullptr));
        } else if (p.kind == "vec2") {
            auto f = parse_floats(v);
            ctx->emit_vec2(ctx->store, ctx->node_id, nm, f[0], f[1]);
        } else if (p.kind == "vec3") {
            auto f = parse_floats(v);
            ctx->emit_vec3(ctx->store, ctx->node_id, nm, f[0], f[1], f[2]);
        } else if (p.kind == "vec4") {
            auto f = parse_floats(v);
            ctx->emit_vec4(ctx->store, ctx->node_id, nm, f[0], f[1], f[2], f[3]);
        } else if (p.kind == "quat") {
            auto f = parse_floats(v);
            ctx->emit_quat(ctx->store, ctx->node_id, nm, f[0], f[1], f[2], f[3]);
        }
    }
}

void RemoteNode::deserialize(const char* params_json) {
    if (!params_json || !*params_json) return;
    // Forward param objects wholesale; provider applies per its own schema.
    peer_->set_params(uid_, params_json);
    last_params_ = params_json;
}

std::string RemoteNode::serialize() const {
    return last_params_.empty() ? "{}" : last_params_;
}
