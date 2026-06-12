// Copyright 2026 Travis West
#include "audio_region.hpp"
#include "audio_engine.hpp"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <optional>

// Block-scheduler tracing: SYGALDREYE_BLOCK_DEBUG=1 on host;
// `adb shell setprop debug.sygaldreye.blk 1` on device.
#ifdef __ANDROID__
#include <android/log.h>
#include <sys/system_properties.h>
static bool blk_debug() {
    static const bool on = [] {
        char v[PROP_VALUE_MAX] = {};
        __system_property_get("debug.sygaldreye.blk", v);
        return v[0] == '1';
    }();
    return on;
}
#define BLKLOG(...) __android_log_print(ANDROID_LOG_INFO, "eyeballs", __VA_ARGS__)
#else
static bool blk_debug() {
    static const bool on = std::getenv("SYGALDREYE_BLOCK_DEBUG") != nullptr;
    return on;
}
#define BLKLOG(...) std::fprintf(stderr, __VA_ARGS__)
#endif

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
    dac_out_keys_.clear();

    if (!g.plan) g.plan = build_plan(g);
    wire_plan(g);   // idempotent; migrated instances need fresh src pointers
    graph_ = &g;
    plan_  = g.plan.get();

    // The ENGINE owns the device stream; it persists across every graph —
    // an empty block region just renders silence. No churn, ever.
    if (plan_->block_order.empty()) return;
    for (std::size_t i : plan_->block_order)
        if (std::string_view{g.nodes[i].desc->type_name} == "dac")
            dac_out_keys_.push_back(g.nodes[i].id + ".audio_out");

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

    auto& engine = AudioEngine::instance();
    engine.enable_device = enable_device;
    engine.ensure_output(
        [this](float* out, int frames) { render_block(out, frames); });
    if (blk_debug())
        BLKLOG("[reb] block=%zu device=%d latches=%zu snaps=%zu",
               plan_->block_order.size(), int(engine.has_device()),
               latches_.size(), snaps_.size());
}

bool AudioRegion::has_device() const {
    return AudioEngine::instance().has_device();
}

// The block scheduler: the same plan subset, the same primitives, a
// block-local store, block cadence.
void AudioRegion::render_block(float* out, int frames) {
    std::memset(out, 0, std::size_t(frames) * 2 * sizeof(float));
    std::unique_lock<std::mutex> lock(plan_mutex_, std::try_to_lock);
    static int dbg_calls = 0;
    if (blk_debug() && (++dbg_calls % 188) == 0)
        BLKLOG("[blk-pre] call=%d lock=%d plan=%d block=%zu",
               dbg_calls, int(lock.owns_lock()), int(plan_ != nullptr),
               plan_ ? plan_->block_order.size() : std::size_t(0));
    if (!lock.owns_lock() || !plan_ || plan_->block_order.empty()) return;

    static int dbg_count = 0;
    bool dbg_now = blk_debug() && (++dbg_count % 188 == 0);
    if (dbg_now) {
        BLKLOG("[blk] t=%.1f frames=%d order=%zu store=%zu latches=%zu snaps=%zu",
               t_, frames, plan_->block_order.size(), store_.size(),
               latches_.size(), snaps_.size());
        for (auto& l : latches_)
            BLKLOG("[blk]   latch ->%s.%s set=%d", l->to.id.c_str(),
                   l->edge->to_port.c_str(), int(l->set));
        for (std::size_t idx : plan_->block_order) {
            auto& n = graph_->nodes[idx];
            auto it = store_.find(n.id + ".audio");
            float peak = 0.f;
            if (it != store_.end())
                if (auto* a = std::get_if<AudioBuffer>(&it->second))
                    for (int i = 0; i < std::min(a->frames * a->channels, 256); ++i)
                        peak = std::max(peak, std::abs(a->data[i]));
            BLKLOG("[blk]   node %s peak=%g", n.id.c_str(), double(peak));
        }
    }

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

    // Deliver EVERY dac's gained output to the device, summed — multiple
    // dacs sum-mix like Pd (mono duplicates to both ears; stereo adds
    // interleaved).
    for (const auto& key : dac_out_keys_)
        if (auto it = store_.find(key); it != store_.end())
            if (auto* a = std::get_if<AudioBuffer>(&it->second)) {
                int n = std::min(frames, a->frames);
                if (a->channels >= 2) {
                    for (int i = 0; i < n * 2; ++i) out[i] += a->data[i];
                } else {
                    for (int i = 0; i < n; ++i) {
                        out[i * 2]     += a->data[i];
                        out[i * 2 + 1] += a->data[i];
                    }
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
    auto& engine = AudioEngine::instance();
    // Zombie-stream recovery: a disconnected device stream never calls
    // back again, which silently stalls the whole block region. Recreate.
    engine.recover_if_dead();
    engine.recover_input();
    static int dbg_pump = 0;
    if (blk_debug() && (++dbg_pump % 600) == 0)
        BLKLOG("[pump] device=%d active=%d",
               int(engine.has_device()), int(active()));
    if (engine.has_device() || !active()) return;
    int frames = std::clamp(int(dt * kRate), 0, 4800);
    if (frames == 0) return;
    static thread_local std::vector<float> buf;
    buf.resize(std::size_t(frames) * 2);
    render_block(buf.data(), frames);
}
