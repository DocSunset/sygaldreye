// Copyright 2026 Travis West
#include "audio_region.hpp"
#include <algorithm>
#include <cstring>
#include <optional>

namespace {
constexpr int    kRate    = 48000;
constexpr size_t kRingCap = 48000 * 2;  // 2 s per crossing edge
} // namespace

void AudioRegion::rebuild(Graph& g) {
    std::lock_guard<std::mutex> lock(plan_mutex_);
    rebuild_unlocked(g);
}

void AudioRegion::rebuild_unlocked(Graph& g) {
    latches_.clear();
    rings_.clear();
    snaps_.clear();
    queues_.clear();
    store_.clear();
    dac_out_key_.clear();

    if (!g.plan) g.plan = build_plan(g);
    wire_plan(g);   // idempotent; migrated instances need fresh src pointers
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
        else if (c.mapping == "queue") {
            auto q = std::make_unique<EvQueue>();
            q->edge       = c.edge;
            q->from_key   = c.edge->from_node + "." + c.edge->from_port;
            q->into_block = c.to_region == port_types::Rate::Block;
            if (q->into_block) {
                auto t = by_id.find(c.edge->to_node);
                if (t == by_id.end()) continue;
                q->to = *t->second;
            }
            queues_.push_back(std::move(q));
        }
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
    for (auto& l : latches_) {
        std::optional<PortValue> v;
        if (l->m.try_lock()) {
            if (l->set) v = l->value;
            l->m.unlock();
        }
        if (v) apply_value(l->to, l->edge->to_port.c_str(), *v);
    }
    for (auto& q : queues_) {
        if (!q->into_block || !q->to.desc->set_scalar_in) continue;
        if (q->pending.load(std::memory_order_acquire) > 0) {
            q->pending.fetch_sub(1, std::memory_order_acq_rel);
            q->to.desc->set_scalar_in(q->to.data, q->edge->to_port.c_str(), 1.0);
            q->applied_high = true;
        } else if (q->applied_high) {
            q->to.desc->set_scalar_in(q->to.data, q->edge->to_port.c_str(), 0.0);
            q->applied_high = false;
        }
    }

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

    // Deliver the dac's gained output to the device (mono duplicates to
    // both ears; stereo passes through interleaved).
    if (auto it = store_.find(dac_out_key_); it != store_.end())
        if (auto* a = std::get_if<AudioBuffer>(&it->second)) {
            int n = std::min(frames, a->frames);
            if (a->channels >= 2) {
                std::memcpy(out, a->data, std::size_t(n) * 2 * sizeof(float));
            } else {
                for (int i = 0; i < n; ++i)
                    out[i * 2] = out[i * 2 + 1] = a->data[i];
            }
        }

    // Crossing mappings: ring pushes + scalar snapshots.
    for (auto& r : rings_) {
        auto it = store_.find(r->from_key);
        if (it == store_.end()) continue;
        auto* a = std::get_if<AudioBuffer>(&it->second);
        if (!a) continue;
        r->sample_rate = a->sample_rate;
        int ch = std::max(1, a->channels);
        r->channels.store(ch, std::memory_order_relaxed);
        size_t head = r->head.load(std::memory_order_relaxed);
        size_t tail = r->tail.load(std::memory_order_acquire);
        std::size_t samples = std::size_t(a->frames) * std::size_t(ch);
        for (std::size_t i = 0; i < samples; ++i) {
            if (head - tail >= kRingCap) break;  // full: drop newest
            r->buf[head % kRingCap] = a->data[i];
            ++head;
        }
        r->head.store(head, std::memory_order_release);
    }
    for (auto& s : snaps_)
        if (auto it = store_.find(s->from_key); it != store_.end())
            if (auto* v = std::get_if<double>(&it->second))
                s->value.store(*v, std::memory_order_relaxed);
    for (auto& q : queues_)
        if (!q->into_block)
            if (auto it = store_.find(q->from_key); it != store_.end())
                if (auto* v = std::get_if<double>(&it->second); v && *v > 0.5)
                    q->pending.fetch_add(1, std::memory_order_acq_rel);
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
        int ch = std::max(1, r->channels.load(std::memory_order_relaxed));
        // Trim to whole frames so consumers never see a ragged tail.
        int frames = int(r->drained.size()) / ch;
        g.values[r->from_key] = AudioBuffer{r->drained.data(), frames, ch,
                                            r->sample_rate};
    }
    for (auto& s : snaps_)
        g.values[s->from_key] = s->value.load(std::memory_order_relaxed);
    for (auto& q : queues_) {
        if (q->into_block) continue;
        if (q->pending.load(std::memory_order_acquire) > 0) {
            q->pending.fetch_sub(1, std::memory_order_acq_rel);
            g.values[q->from_key] = 1.0;
        } else {
            g.values[q->from_key] = 0.0;
        }
    }
}

void AudioRegion::capture_latches(const Graph& g) {
    for (auto& q : queues_) {
        if (!q->into_block) continue;
        auto it = g.values.find(q->from_key);
        if (it == g.values.end()) continue;
        if (auto* v = std::get_if<double>(&it->second); v && *v > 0.5)
            q->pending.fetch_add(1, std::memory_order_acq_rel);
    }
    for (auto& l : latches_) {
        auto it = g.values.find(l->edge->from_node + "." + l->edge->from_port);
        if (it == g.values.end()) continue;
        std::lock_guard<std::mutex> lock(l->m);
        l->value = it->second;
        l->set   = true;
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
