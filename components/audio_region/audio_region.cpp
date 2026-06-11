// Copyright 2026 Travis West
#include "audio_region.hpp"
#include <algorithm>
#include <cstring>

namespace {
constexpr int    kRate    = 48000;
constexpr size_t kRingCap = 48000 * 2;  // 2 s per crossing edge
} // namespace

void AudioRegion::rebuild(Graph& g) {
    std::lock_guard<std::mutex> lock(plan_mutex_);
    latches_.clear();
    rings_.clear();
    snaps_.clear();
    store_.clear();
    dac_out_key_.clear();

    if (!g.plan) g.plan = build_plan(g);
    graph_ = &g;
    plan_  = g.plan.get();

    if (plan_->block_order.empty()) {
        stream_.reset();
        return;
    }
    for (std::size_t i : plan_->block_order)
        if (std::string_view{g.nodes[i].desc->type_name} == "dac" &&
            dac_out_key_.empty())
            dac_out_key_ = g.nodes[i].id + ".out";

    // Instantiate the canonical mappings the plan declared at crossings.
    std::unordered_map<std::string, const NodeInstance*> by_id;
    for (auto& n : g.nodes) by_id[n.id] = &n;
    for (const auto& c : plan_->crossings) {
        if (c.mapping == "latch") {
            auto it = by_id.find(c.edge->to_node);
            if (it == by_id.end()) continue;
            auto l = std::make_unique<Latch>();
            l->edge = c.edge;
            l->to   = *it->second;
            latches_.push_back(std::move(l));
        } else if (c.mapping == "ring") {
            auto r = std::make_unique<Ring>();
            r->from_key = c.edge->from_node + "." + c.edge->from_port;
            r->buf.resize(kRingCap);
            rings_.push_back(std::move(r));
        } else if (c.mapping == "snapshot") {
            auto s = std::make_unique<Snap>();
            s->from_key = c.edge->from_node + "." + c.edge->from_port;
            snaps_.push_back(std::move(s));
        }
        // "queue" (event) crossings: not yet instantiated here — events
        // into/out of the block region land with block-rate event work.
    }

    if (!stream_ && enable_device) {
        stream_ = AudioOutput::create(
            [this](float* out, int frames) { render_block(out, frames); }, kRate);
        if (stream_) stream_->start();
    }
}

// The block scheduler: the same plan subset, the same primitives, a
// block-local store, block cadence.
void AudioRegion::render_block(float* out, int frames) {
    std::memset(out, 0, std::size_t(frames) * 2 * sizeof(float));
    std::unique_lock<std::mutex> lock(plan_mutex_, std::try_to_lock);
    if (!lock.owns_lock() || !plan_ || plan_->block_order.empty()) return;

    t_ += double(frames) / kRate;
    for (auto& l : latches_)
        if (l->set.load(std::memory_order_relaxed) && l->to.desc->set_scalar_in)
            l->to.desc->set_scalar_in(l->to.data, l->edge->to_port.c_str(),
                                      l->value.load(std::memory_order_relaxed));

    for (std::size_t idx : plan_->block_order) {
        auto& n = graph_->nodes[idx];
        for (auto& a : plan_->appliers[idx])
            if (const PortValue* src = resolve_applier(a, store_))
                apply_value(n, a.edge->to_port.c_str(), *src);
        for (auto* d : plan_->delayed[idx])
            if (d->prev)
                apply_value(n, d->applier.edge->to_port.c_str(), *d->prev);
        if (n.desc->process) n.desc->process(n.data, t_);
        if (n.desc->push_outputs) {
            EyeballsOutputCtx ctx = output_ctx(&store_, n.id.c_str());
            n.desc->push_outputs(n.data, &ctx);
        }
    }
    for (auto& d : plan_->delays)
        if (d.region == port_types::Rate::Block)
            if (const PortValue* src = resolve_applier(d.applier, store_))
                d.prev = *src;

    // Deliver the dac's gained output to the device (mono → stereo).
    if (auto it = store_.find(dac_out_key_); it != store_.end())
        if (auto* a = std::get_if<AudioBuffer>(&it->second)) {
            int n = std::min(frames, a->frames);
            for (int i = 0; i < n; ++i)
                out[i * 2] = out[i * 2 + 1] = a->data[i];
        }

    // Crossing mappings: ring pushes + scalar snapshots.
    for (auto& r : rings_) {
        auto it = store_.find(r->from_key);
        if (it == store_.end()) continue;
        auto* a = std::get_if<AudioBuffer>(&it->second);
        if (!a) continue;
        r->sample_rate = a->sample_rate;
        size_t head = r->head.load(std::memory_order_relaxed);
        size_t tail = r->tail.load(std::memory_order_acquire);
        int stride = std::max(1, a->channels);
        for (int i = 0; i < a->frames; ++i) {
            if (head - tail >= kRingCap) break;  // full: drop newest
            r->buf[head % kRingCap] = a->data[std::size_t(i) * std::size_t(stride)];
            ++head;
        }
        r->head.store(head, std::memory_order_release);
    }
    for (auto& s : snaps_)
        if (auto it = store_.find(s->from_key); it != store_.end())
            if (auto* v = std::get_if<double>(&it->second))
                s->value.store(*v, std::memory_order_relaxed);
}

void AudioRegion::publish(Graph& g) {
    for (auto& r : rings_) {
        size_t tail = r->tail.load(std::memory_order_relaxed);
        size_t head = r->head.load(std::memory_order_acquire);
        r->drained.clear();
        while (tail < head) {
            r->drained.push_back(r->buf[tail % kRingCap]);
            ++tail;
        }
        r->tail.store(tail, std::memory_order_release);
        g.values[r->from_key] = AudioBuffer{r->drained.data(),
                                            int(r->drained.size()), 1,
                                            r->sample_rate};
    }
    for (auto& s : snaps_)
        g.values[s->from_key] = s->value.load(std::memory_order_relaxed);
}

void AudioRegion::capture_latches(const Graph& g) {
    for (auto& l : latches_) {
        auto it = g.values.find(l->edge->from_node + "." + l->edge->from_port);
        if (it == g.values.end() || !std::holds_alternative<double>(it->second))
            continue;
        l->value.store(std::get<double>(it->second), std::memory_order_relaxed);
        l->set.store(true, std::memory_order_relaxed);
    }
}

void AudioRegion::pump_offline(double dt) {
    if (stream_ || !active()) return;
    int frames = std::clamp(int(dt * kRate), 0, 4800);
    if (frames == 0) return;
    static thread_local std::vector<float> buf;
    buf.resize(std::size_t(frames) * 2);
    render_block(buf.data(), frames);
}
