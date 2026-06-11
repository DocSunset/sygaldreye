// Copyright 2026 Travis West
#pragma once
#include "signal_graph.hpp"
#include "audio_output.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// The block region (edge_executor.design.md step 5). Nodes reachable
// upstream of a `dac` through audio edges tick in the audio callback at
// block rate; everything else stays in the render region. Boundaries are
// the canonical mappings, reified here:
//   frame → block scalar : latch     (atomic, captured at block start)
//   block → frame scalar : snapshot  (atomic, published each render frame)
//   block → frame audio  : ring      (SPSC, drained each render frame and
//                                     republished into Graph::values so
//                                     ordinary appliers deliver it)
// No device? pump_offline drives blocks from the render thread — audio
// graphs keep computing on headless peers (and feed the spectrogram).
class AudioRegion {
public:
    // Render thread, after every graph swap (and once at init). Marks the
    // block nodes in g.offrender so the render TickPlan excludes them.
    void rebuild(Graph& g);

    // Render thread, BEFORE tick_graph: publish rings + scalar snapshots
    // into g.values.
    void publish(Graph& g);
    // Render thread, AFTER tick_graph: capture latch sources from g.values.
    void capture_latches(const Graph& g);
    // Render thread: drive blocks manually when no device stream exists.
    void pump_offline(double dt);

    bool has_device() const { return stream_.has_value(); }
    bool active() const { return !order_.empty(); }
    bool enable_device = true;  // tests/headless force the offline pump

private:
    void render_block(float* out, int frames);  // audio thread

    struct Latch {                       // frame → block scalar
        std::string         from_key;    // producer key in g.values
        NodeInstance        to;          // block node (copy)
        std::string         to_port;
        std::atomic<double> value{0.0};
        std::atomic<bool>   set{false};
    };
    struct Ring {                        // block → frame audio
        std::string        from_key;
        std::vector<float> buf;          // SPSC ring
        std::atomic<size_t> head{0}, tail{0};
        std::vector<float> drained;      // frame-side view storage
        int                sample_rate = 48000;
    };
    struct Snap {                        // block → frame scalar
        std::string         from_key;
        std::atomic<double> value{0.0};
    };

    std::mutex                  plan_mutex_;  // callback try_locks; swap holds
    std::vector<NodeInstance>   order_;       // block nodes, topo order (copies)
    std::vector<Edge>           internal_;    // edges within the region
    std::vector<std::unique_ptr<Latch>> latches_;
    std::vector<std::unique_ptr<Ring>>  rings_;
    std::vector<std::unique_ptr<Snap>>  snaps_;
    std::string                 dac_out_key_;
    std::unordered_map<std::string, AudioBuffer> store_;   // block-local values
    std::unordered_map<std::string, double>      scalars_;
    double                      t_ = 0.0;
    std::optional<AudioOutput>  stream_;
};
