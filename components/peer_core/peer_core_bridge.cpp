// Copyright 2026 Travis West
// Net mapping (edge_executor.design.md step 6). Provider side: advertise
// node types over /ws, spawn consumer-requested nodes into the LIVE local
// graph (advertisement = capability = placement), apply forwarded inputs,
// mirror outputs back every frame. Consumer side: connect_peer registers
// proxy descriptors. The evaluator can't tell a proxy from a local node.
#include "peer_core.hpp"
#include "port_schema_reader.hpp"
#include "signal_graph_plan.hpp"
#include <array>
#include <optional>
#include <cstdio>
#include <cstdlib>

namespace {

std::string_view find_string(std::string_view json, std::string_view key) {
    std::string needle = "\"" + std::string(key) + "\":\"";
    auto p = json.find(needle);
    if (p == std::string_view::npos) return {};
    p += needle.size();
    auto e = json.find('"', p);
    return (e == std::string_view::npos) ? std::string_view{} : json.substr(p, e - p);
}

std::string_view find_object(std::string_view json, std::string_view key) {
    std::string needle = "\"" + std::string(key) + "\":{";
    auto p = json.find(needle);
    if (p == std::string_view::npos) return {};
    p += needle.size() - 1;
    int depth = 0;
    for (auto i = p; i < json.size(); ++i) {
        if (json[i] == '{') ++depth;
        else if (json[i] == '}' && --depth == 0) return json.substr(p, i - p + 1);
    }
    return {};
}

// Insert a node entry into a serialized graph (same surgery as spawner).
std::string insert_node(std::string json, const std::string& id,
                        const std::string& type, std::string_view params) {
    auto epos = json.find("\"edges\"");
    auto pos  = (epos == std::string::npos) ? json.rfind(']') : json.rfind(']', epos);
    if (pos == std::string::npos) return json;
    std::string entry = (pos > 0 && json[pos - 1] != '[') ? "," : "";
    entry += "{\"id\":\"" + id + "\",\"type\":\"" + type + "\"";
    if (!params.empty()) { entry += ",\"params\":"; entry += params; }
    entry += "}";
    json.insert(pos, entry);
    return json;
}

// Remove the node object whose id matches (consumer-spawned nodes have no
// local edges; a dangling edge is skipped by the plan anyway).
std::string remove_node_entry(std::string json, const std::string& id) {
    auto p = json.find("\"id\":\"" + id + "\"");
    if (p == std::string::npos) return json;
    auto start = json.rfind('{', p);
    if (start == std::string::npos) return json;
    int depth = 0;
    std::size_t end = start;
    for (std::size_t i = start; i < json.size(); ++i) {
        if (json[i] == '{') ++depth;
        else if (json[i] == '}' && --depth == 0) { end = i; break; }
    }
    if (start > 0 && json[start - 1] == ',') --start;
    else if (end + 1 < json.size() && json[end + 1] == ',') ++end;
    json.erase(start, end - start + 1);
    return json;
}

std::array<float, 4> parse_floats(std::string_view s, int* n_out) {
    std::array<float, 4> out{};
    std::size_t i = 0;
    int n = 0;
    while (i < s.size() && n < 4) {
        while (i < s.size() && (s[i] == '[' || s[i] == ',' || s[i] == ' ')) ++i;
        if (i >= s.size() || s[i] == ']') break;
        out[std::size_t(n++)] = std::strtof(std::string(s.substr(i)).c_str(), nullptr);
        while (i < s.size() && s[i] != ',' && s[i] != ']') ++i;
    }
    *n_out = n;
    return out;
}

void append_value(std::string& out, const PortValue& val) {
    char buf[120];
    if (std::holds_alternative<double>(val)) {
        std::snprintf(buf, sizeof(buf), "%g", std::get<double>(val));
    } else if (std::holds_alternative<Eigen::Vector2f>(val)) {
        auto& v = std::get<Eigen::Vector2f>(val);
        std::snprintf(buf, sizeof(buf), "[%g,%g]", double(v.x()), double(v.y()));
    } else if (std::holds_alternative<Eigen::Vector3f>(val)) {
        auto& v = std::get<Eigen::Vector3f>(val);
        std::snprintf(buf, sizeof(buf), "[%g,%g,%g]",
                      double(v.x()), double(v.y()), double(v.z()));
    } else if (std::holds_alternative<Eigen::Vector4f>(val)) {
        auto& v = std::get<Eigen::Vector4f>(val);
        std::snprintf(buf, sizeof(buf), "[%g,%g,%g,%g]",
                      double(v.x()), double(v.y()), double(v.z()), double(v.w()));
    } else if (std::holds_alternative<Eigen::Quaternionf>(val)) {
        auto& v = std::get<Eigen::Quaternionf>(val);
        std::snprintf(buf, sizeof(buf), "[%g,%g,%g,%g]",
                      double(v.x()), double(v.y()), double(v.z()), double(v.w()));
    } else {
        return;  // textures/draw calls/meshes/audio don't mirror (v1)
    }
    out += buf;
}

} // namespace

std::string PeerCore::handle_ws(unsigned long conn_id, std::string_view msg) {
    auto op = find_string(msg, "op");
    if (op == "types") {
        std::string out = R"({"op":"types","types":[)";
        bool first = true;
        for (const auto& name : registry.type_names()) {
            if (name.find('@') != std::string::npos) continue;  // no chaining
            const auto* d = registry.find(name);
            if (!d || !d->create) continue;
            if (!first) out += ',';
            first = false;
            out += "{\"type\":\"" + name + "\",\"schema\":";
            out += d->port_schema ? d->port_schema : "{}";
            out += '}';
        }
        return out + "]}";
    }
    std::string uid{find_string(msg, "id")};
    if (uid.empty()) return {};
    if (op == "spawn") {
        std::string type{find_string(msg, "type")};
        if (!registry.find(type)) return {};
        {
            std::lock_guard<std::mutex> lock(hosted_mutex_);
            hosted_[uid] = conn_id;
        }
        std::lock_guard<std::mutex> lock(graph_mutex_);
        if (active_)
            queue_edit(insert_node(serialize_graph(*active_), uid, type,
                                   find_object(msg, "params")));
    } else if (op == "set") {
        remote_sets_.push({uid, std::string(find_object(msg, "params"))});
    } else if (op == "despawn") {
        {
            std::lock_guard<std::mutex> lock(hosted_mutex_);
            hosted_.erase(uid);
        }
        std::lock_guard<std::mutex> lock(graph_mutex_);
        if (active_) queue_edit(remove_node_entry(serialize_graph(*active_), uid));
    }
    return {};
}

// Render thread (from begin_frame): apply forwarded inputs to hosted nodes.
// deserialize covers scalar/text params; arrays become PortValues applied
// through the shared executor dispatch, typed by the node's own schema.
void PeerCore::apply_remote_sets() {
    for (auto& [uid, params] : remote_sets_.drain()) {
        for (auto& n : active_->nodes) {
            if (n.id != uid) continue;
            if (n.desc->deserialize) n.desc->deserialize(n.data, params.c_str());
            PortSchema schema = parse_port_schema(n.desc->port_schema);
            for (const auto& p : schema.inputs) {
                auto needle = "\"" + p.name + "\":[";
                auto pos = params.find(needle);
                if (pos == std::string::npos) continue;
                int cnt = 0;
                auto f = parse_floats(
                    std::string_view(params).substr(pos + needle.size() - 1), &cnt);
                std::optional<PortValue> v;
                if (p.kind == "vec2")      v = Eigen::Vector2f{f[0], f[1]};
                else if (p.kind == "vec3") v = Eigen::Vector3f{f[0], f[1], f[2]};
                else if (p.kind == "vec4") v = Eigen::Vector4f{f[0], f[1], f[2], f[3]};
                else if (p.kind == "quat") v = Eigen::Quaternionf{f[3], f[0], f[1], f[2]};
                if (v) apply_value(n, p.name.c_str(), *v);
            }
            break;
        }
    }
}

// Render thread (from tick, after the values snapshot): mirror hosted
// nodes' outputs to their consumers — value-rate frames, latest wins.
void PeerCore::push_remote_outs() {
    std::map<std::string, unsigned long> hosted;
    {
        std::lock_guard<std::mutex> lock(hosted_mutex_);
        hosted = hosted_;
    }
    if (hosted.empty()) return;
    std::lock_guard<std::mutex> lock(values_mutex_);
    for (const auto& [uid, conn] : hosted) {
        std::string prefix = uid + ".";
        std::string vals;
        for (const auto& [key, val] : values_snapshot_) {
            if (key.compare(0, prefix.size(), prefix) != 0) continue;
            std::string entry;
            append_value(entry, val);
            if (entry.empty()) continue;
            if (!vals.empty()) vals += ',';
            vals += "\"" + key.substr(prefix.size()) + "\":" + entry;
        }
        if (vals.empty()) continue;
        http_.ws_send(conn, R"({"op":"out","id":")" + uid + R"(","values":{)" +
                                vals + "}}");
    }
}

int PeerCore::connect_peer(const std::string& ws_url) {
    // alias = host:port from ws://host:port/ws
    std::string alias = ws_url;
    if (auto p = alias.find("://"); p != std::string::npos) alias = alias.substr(p + 3);
    if (auto p = alias.find('/'); p != std::string::npos) alias = alias.substr(0, p);

    auto peer = std::make_unique<RemotePeer>(ws_url, alias);
    if (!peer->fetch_types()) return 0;
    int count = 0;
    for (const auto& t : peer->types()) {
        auto rd = std::make_unique<RemoteDescriptor>(peer.get(), t.type + "@" + alias,
                                                     t.type, t.schema);
        registry.register_builtin(rd->descriptor());
        remote_types_.push_back(std::move(rd));
        ++count;
    }
    remote_peers_.push_back(std::move(peer));
    return count;
}
