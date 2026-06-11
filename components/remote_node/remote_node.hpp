// Copyright 2026 Travis West
#pragma once
#include "eyeballs_node_abi.h"
#include "ws_link.hpp"
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Consumer half of the net mapping: a peer advertises the NODES it
// supports; each advertised type becomes a local proxy descriptor
// ("lfo@host:port"). Instantiating the proxy spawns the real node in the
// provider's own live graph; inputs forward as coalesced value frames,
// outputs mirror back every provider frame. The evaluator can't tell.

// One provider connection: the link, the advertisement, and the mirrored
// output store for every proxy instance spawned through it.
class RemotePeer {
public:
    RemotePeer(std::string ws_url, std::string alias);

    // Blocks (≤ timeout) for the provider's type advertisement.
    bool fetch_types(int timeout_ms = 3000);
    struct TypeInfo { std::string type, schema; };
    const std::vector<TypeInfo>& types() const { return types_; }
    const std::string& alias() const { return alias_; }

    void spawn(const std::string& uid, const std::string& remote_type,
               const std::string& params_json);
    void set_params(const std::string& uid, const std::string& params_json);
    void despawn(const std::string& uid);
    std::string latest(const std::string& uid);  // raw values JSON object

private:
    void handle(std::string_view msg);

    WsLink                             link_;
    std::string                        alias_;
    std::vector<TypeInfo>              types_;
    std::atomic_bool                   types_received_{false};
    std::mutex                         m_;
    std::map<std::string, std::string> latest_;  // uid → values JSON
};

// The proxy node instance (descriptor `data`).
class RemoteNode {
public:
    RemoteNode(RemotePeer* peer, std::string remote_type, std::string schema);
    ~RemoteNode();
    void set_in(const std::string& port, std::string value_json);  // coalesced
    void process(double time_s);
    void push_outputs(EyeballsOutputCtx* ctx);
    void deserialize(const char* params_json);
    std::string serialize() const;

private:
    RemotePeer*                        peer_;
    std::string                        remote_type_, schema_, uid_;
    std::map<std::string, std::string> pending_;  // port → value JSON
    std::string                        last_params_;
    bool                               dirty_ = false;
};

// Slot-trampoline descriptor for one remote type (same pattern as
// SubgraphDescriptor — create() can't capture).
struct RemoteDescriptor {
    RemoteDescriptor(RemotePeer* peer, std::string local_name,
                     std::string remote_type, std::string schema_json);
    ~RemoteDescriptor();
    RemoteDescriptor(const RemoteDescriptor&)            = delete;
    RemoteDescriptor& operator=(const RemoteDescriptor&) = delete;

    const EyeballsNodeDescriptor* descriptor() const { return &desc_; }
    RemoteNode* make() const;

private:
    RemotePeer*            peer_;
    std::string            local_name_, remote_type_, schema_;
    EyeballsNodeDescriptor desc_{};
    int                    slot_ = -1;
};
