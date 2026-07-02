// Copyright 2026 Travis West
#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include "audio_region.hpp"
#include "component_registry.hpp"
#include "editor_layout.hpp"
#include "event_queue.hpp"
#include "http_server.hpp"
#include "remote_node.hpp"
#include "signal_graph.hpp"

// The portable peer: everything a running instance is, minus the platform.
// Owns the registry, the live/pending graph pair (swap + migrate), the
// queue mappings for params and edits, the full HTTP control surface, the
// values snapshot, and screenshot fulfilment. Host and Android are thin
// shells over this — same application, two build targets, differing only
// in registered nodes, default graph, and frame pump.
struct PeerCore {
    ComponentRegistry registry;

    struct Config {
        int http_port = 8080;
        std::string default_graph_json;
        std::string data_dir = "/tmp";  // plugins, screenshots, wavs
        std::string graphs_dir;         // *.json subgraph plugins ("" = none)
    };

    // Call after the shell has populated `registry` (and, for nodes that
    // lazily build GL, after a context exists on the calling thread).
    void init(const Config& cfg);

    // ── frame phases, render thread ──────────────────────────────────────
    void begin_frame();                // swap+migrate pending, apply params
    void pump_contexts(float aspect);  // v9 host-context hook + fly_camera aspect
    void tick(double time_s);          // tick graph, snapshot values
    void collect_edits();              // edit queue → pending graph
    // At the shell's safe readback point (current READ framebuffer).
    void fulfil_screenshot(int width, int height);

    Graph* graph() { return active_.get(); }  // render thread only

    // ── any thread ───────────────────────────────────────────────────────
    void queue_param(std::string node_id, std::string params_json);
    void queue_edit(std::string graph_json);
    // Pull observability (phase D): probe blocks until the next frame
    // boundary publishes a fresh snapshot. HTTP threads only — the render
    // thread reads node storage via read_node_output instead.
    std::optional<PortValue> probe(const std::string& key);
    // Direct typed read of a node's output storage (render thread).
    std::optional<PortValue> read_node_output(
        const std::string& node_id, const std::string& port, const std::string& kind);
    bool quit_requested() const { return quit_.load(); }

    // Net mapping, consumer side: connect to another peer's /ws, fetch its
    // advertised node types, register each as a local proxy descriptor
    // named "<type>@<host:port>". Returns the number of types registered.
    int connect_peer(const std::string& ws_url);

    // Fires on the render thread when a pending graph becomes active
    // (after migration) — e.g. the device editor refreshes its cards.
    std::function<void(const Graph*)> on_graph_swapped;

private:
    void install_routes();
    std::string handle_ws(unsigned long conn_id, std::string_view msg);
    void apply_remote_sets();
    void push_remote_outs();

    Config cfg_;
    HttpServer http_;
    std::mutex graph_mutex_;
    std::unique_ptr<Graph> active_;
    std::unique_ptr<Graph> pending_;

    EventQueue<std::pair<std::string, std::string>> param_events_;
    EventQueue<std::string> edit_events_;

    std::mutex values_mutex_;
    std::condition_variable values_cv_;
    bool values_requested_ = false;
    std::uint64_t values_gen_ = 0;
    std::unordered_map<std::string, PortValue> values_snapshot_;

    std::mutex shot_mutex_;
    std::condition_variable shot_cv_;
    std::string shot_path_;  // non-empty: a request is pending
    bool shot_done_ = false;
    bool shot_ok_ = false;

    std::string meta_graph_json_;
    std::mutex meta_mutex_;
    std::atomic<long> play_seq_{0};
    std::atomic_bool quit_{false};

    // Net mapping state. Provider side: nodes spawned here on behalf of
    // consumers (uid → ws connection to mirror outputs to) + typed input
    // sets queued for the render thread. Consumer side: peer links and the
    // proxy descriptors registered from their advertisements.
    // Editor seam (vr_editor_decomposition.md S3): shared per-id card-position
    // overrides (graph_source reads, wire_drag writes) + the sorted registry
    // type list (the palette's source). Owned here, pointed at the editor's
    // resource-holder nodes in pump_contexts.
    editor_layout::PosOverrides editor_overrides_;
    std::vector<std::string> sorted_types_;
    // Shared memoized editor layout + its generation stamps: graph_gen_
    // bumps on every swap and in-place param write, overrides_gen_ when a
    // card drag writes the override map. Editor nodes rebuild layout (and
    // undo_node re-serializes) only when a stamp moves.
    editor_layout::LayoutCache layout_cache_;
    std::uint64_t graph_gen_ = 0;
    std::uint64_t overrides_gen_ = 0;
    AudioRegion audio_;
    double prev_tick_t_ = 0.0;
    std::mutex hosted_mutex_;
    std::map<std::string, unsigned long> hosted_;
    EventQueue<std::pair<std::string, std::string>> remote_sets_;
    std::vector<std::unique_ptr<RemotePeer>> remote_peers_;
    std::vector<std::unique_ptr<RemoteDescriptor>> remote_types_;
};
