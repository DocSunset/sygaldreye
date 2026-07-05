// Copyright 2026 Travis West
#include "remote_node.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#include <utility>

// Same trampoline-pool trick as SubgraphDescriptor: create() takes no
// argument, so each remote type claims a slot carrying its identity.

namespace {

constexpr int kMaxRemoteTypes = 256;
const RemoteDescriptor* g_slots[kMaxRemoteTypes] = {};

void* create_in_slot(int slot) {
    const auto* d = g_slots[slot];
    return d ? d->make() : nullptr;
}

template <int Slot>
void* create_trampoline() { return create_in_slot(Slot); }

template <int... Is>
constexpr auto make_trampoline_table(std::integer_sequence<int, Is...>) {
    return std::array<void* (*)(), sizeof...(Is)>{create_trampoline<Is>...};
}

const auto g_trampolines =
    make_trampoline_table(std::make_integer_sequence<int, kMaxRemoteTypes>{});

int claim_slot(const RemoteDescriptor* d) {
    for (int i = 0; i < kMaxRemoteTypes; ++i)
        if (!g_slots[i]) { g_slots[i] = d; return i; }
    return -1;
}

std::string num(double v) {
    char buf[40];
    std::snprintf(buf, sizeof(buf), "%g", v);
    return buf;
}

} // namespace

RemoteNode* RemoteDescriptor::make() const {
    return new RemoteNode(peer_, remote_type_, schema_);
}

RemoteDescriptor::RemoteDescriptor(RemotePeer* peer, std::string local_name,
                                   std::string remote_type, std::string schema_json)
    : peer_(peer), local_name_(std::move(local_name)),
      remote_type_(std::move(remote_type)), schema_(std::move(schema_json)) {
    slot_ = claim_slot(this);

    desc_.version     = EYEBALLS_ABI_VERSION;
    desc_.type_name   = local_name_.c_str();
    desc_.port_schema = schema_.c_str();
    desc_.create      = (slot_ >= 0) ? g_trampolines[slot_] : nullptr;
    desc_.destroy = [](void* p) { delete static_cast<RemoteNode*>(p); };
    desc_.process = [](void* p, double t) { static_cast<RemoteNode*>(p)->process(t); };
    desc_.push_outputs = [](void* p, EyeballsOutputCtx* ctx) {
        static_cast<RemoteNode*>(p)->push_outputs(ctx);
    };
    desc_.serialize = [](void* p) -> const char* {
        return strdup(static_cast<RemoteNode*>(p)->serialize().c_str());
    };
    desc_.free_str    = [](const char* s) { free(const_cast<char*>(s)); };
    desc_.deserialize = [](void* p, const char* json) {
        static_cast<RemoteNode*>(p)->deserialize(json);
    };
    desc_.set_scalar_in = [](void* p, const char* port, double v) {
        static_cast<RemoteNode*>(p)->set_in(port, num(v));
    };
    desc_.set_vec2_in = [](void* p, const char* port, float x, float y) {
        static_cast<RemoteNode*>(p)->set_in(port, "[" + num(x) + "," + num(y) + "]");
    };
    desc_.set_vec3_in = [](void* p, const char* port, float x, float y, float z) {
        static_cast<RemoteNode*>(p)->set_in(
            port, "[" + num(x) + "," + num(y) + "," + num(z) + "]");
    };
    desc_.set_vec4_in = [](void* p, const char* port, float x, float y, float z, float w) {
        static_cast<RemoteNode*>(p)->set_in(
            port, "[" + num(x) + "," + num(y) + "," + num(z) + "," + num(w) + "]");
    };
    desc_.set_quat_in = [](void* p, const char* port, float x, float y, float z, float w) {
        static_cast<RemoteNode*>(p)->set_in(
            port, "[" + num(x) + "," + num(y) + "," + num(z) + "," + num(w) + "]");
    };
}

RemoteDescriptor::~RemoteDescriptor() {
    if (slot_ >= 0 && g_slots[slot_] == this) g_slots[slot_] = nullptr;
}
