// Copyright 2026 Travis West
#pragma once
#include "signal_graph.hpp"
#include "signal_graph_plan.hpp"
#include "audio_output.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// The block scheduler (edge_executor.design.md step 5). Drives the SAME
// TickPlan as the render scheduler — its block_order subset, with the
// shared appliers / z⁻¹ delays / apply_value / output_ctx primitives —
// against a block-local value store, inside the audio callback. The
// plan's region Crossings (classified by port_types::crossing_mapping)
// are instantiated here as the canonical mappings:
//   latch    (frame → block scalar; atomic, applied at block start)
//   snapshot (block → frame scalar; atomic, published per render frame)
//   ring     (block → frame audio;  SPSC, drained per render frame and
//             republished into Graph::values for ordinary appliers)
// No device? pump_offline drives blocks from the render thread.
class AudioRegion {
public:
    // Render thread, once per graph (builds g.plan if the render scheduler
    // hasn't yet). Instantiates boundary mappings from plan crossings.
    void rebuild(Graph& g);

    // Render thread, BEFORE tick_graph: rings + snapshots → g.values.
    void publish(Graph& g);
    // Render thread, AFTER tick_graph: capture latch sources from g.values.
    void capture_latches(const Graph& g);
    // Render thread: drive blocks manually when no device stream exists.
    void pump_offline(double dt);

    bool has_device() const { return stream_.has_value(); }
    bool active() const { return plan_ && !plan_->block_order.empty(); }
    bool enable_device = true;  // tests/headless force the offline pump

private:
    void render_block(float* out, int frames);  // audio thread

    struct Latch {                       // frame → block scalar
        const Edge*         edge;
        NodeInstance        to;          // block node (copy)
        std::atomic<double> value{0.0};
        std::atomic<bool>   set{false};
    };
    struct Ring {                        // block → frame audio
        std::string         from_key;
        std::vector<float>  buf;         // SPSC ring
        std::atomic<size_t> head{0}, tail{0};
        std::vector<float>  drained;     // frame-side view storage
        int                 sample_rate = 48000;
    };
    struct Snap {                        // block → frame scalar
        std::string         from_key;
        std::atomic<double> value{0.0};
    };

    std::mutex  plan_mutex_;   // callback try_locks; rebuild holds
    Graph*      graph_ = nullptr;
    TickPlan*   plan_  = nullptr;
    std::vector<std::unique_ptr<Latch>> latches_;
    std::vector<std::unique_ptr<Ring>>  rings_;
    std::vector<std::unique_ptr<Snap>>  snaps_;
    std::string dac_out_key_;
    std::unordered_map<std::string, PortValue> store_;  // block-region values
    double                     t_ = 0.0;
    std::optional<AudioOutput> stream_;
};
