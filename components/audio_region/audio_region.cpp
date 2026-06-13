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
    dacs_.clear();

    if (!g.plan) g.plan = build_plan(g);
    wire_plan(g);   // idempotent; migrated instances need fresh src pointers
    graph_ = &g;
    plan_  = g.plan.get();

    // The ENGINE owns the device stream; it persists across every graph —
    // an empty block region just renders silence. No churn, ever.
    if (plan_->block_order.empty()) return;
    for (std::size_t i : plan_->block_order)
        if (std::string_view{g.nodes[i].desc->type_name} == "dac")
            dacs_.push_back(g.nodes[i]);

    // Instantiate the canonical mappings the plan declared at crossings.
    std::unordered_map<std::string, const NodeInstance*> by_id;
    for (auto& n : g.nodes) by_id[n.id] = &n;
    for (const auto& c : plan_->crossings) {
        auto f = by_id.find(c.edge->from_node);
        auto t = by_id.find(c.edge->to_node);
        if (f == by_id.end() || t == by_id.end()) continue;
        if (c.mapping == "latch") {
            auto l = std::make_unique<Latch>();
            l->edge = c.edge;
            l->from = *f->second;
            l->kind = c.payload_kind;
            l->to   = *t->second;
            latches_.push_back(std::move(l));
        } else if (c.mapping == "ring") {
            auto r = std::make_unique<Ring>();
            r->from = *f->second;
            r->port = c.edge->from_port;
            r->buf.resize(kRingCap);
            // The consumer reads the plan-owned slot this ring drains into.
            for (auto& per_node : plan_->slot_appliers)
                for (auto& sa : per_node)
                    if (sa.applier.edge == c.edge) r->slot = sa.audio;
            rings_.push_back(std::move(r));
        } else if (c.mapping == "snapshot") {
            auto s = std::make_unique<Snap>();
            s->edge = c.edge;
            s->from = *f->second;
            s->to   = *t->second;
            snaps_.push_back(std::move(s));
        }
        else if (c.mapping == "queue") {
            auto q = std::make_unique<EvQueue>();
            q->edge       = c.edge;
            q->from       = *f->second;
            q->kind       = c.payload_kind;
            q->to         = *t->second;
            q->into_block = c.to_region == port_types::Rate::Block;
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
    if (!lock.owns_lock() || !plan_ || plan_->block_order.empty()) return;

    // Debug snapshot: snprintf into preallocated storage only — NEVER log
    // from this thread (the syscalls blew the callback deadline; audible).
    static int dbg_count = 0;
    bool dbg_now = blk_debug() && (++dbg_count % 188 == 0) &&
                   !dbg_ready_.load(std::memory_order_relaxed);
    if (dbg_now) {
        int off = std::snprintf(dbg_buf_, sizeof(dbg_buf_),
               "[blk] t=%.1f frames=%d order=%zu latches=%zu snaps=%zu",
               t_, frames, plan_->block_order.size(),
               latches_.size(), snaps_.size());
        for (std::size_t idx : plan_->block_order) {
            auto& n = graph_->nodes[idx];
            // v6 convention: generators emit 'audio', processors/dac
            // 'audio_out' — try both, straight from node storage.
            auto v = read_output(n, "audio", "audio");
            if (!v) v = read_output(n, "audio_out", "audio");
            float peak = 0.f;
            if (v)
                if (auto* a = std::get_if<AudioBuffer>(&*v))
                    for (int i = 0; i < std::min(a->frames * a->channels, 256); ++i)
                        peak = std::max(peak, std::abs(a->data[i]));
            if (off < int(sizeof(dbg_buf_)))
                off += std::snprintf(dbg_buf_ + off, sizeof(dbg_buf_) - off,
                                     "\n[blk]   node %s peak=%g",
                                     n.id.c_str(), double(peak));
        }
        dbg_ready_.store(true, std::memory_order_release);
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
            if (auto src = read_output(a))
                apply_value(n, a.edge->to_port.c_str(), *src);
        for (auto* d : plan_->delayed[idx])
            if (d->prev)
                apply_value(n, d->applier.edge->to_port.c_str(), *d->prev);
        if (n.desc->process) n.desc->process(n.data, t_);
    }
    for (auto& d : plan_->delays)
        if (d.region == port_types::Rate::Block)
            if (auto src = read_output(d.applier))
                d.prev = *src;

    // Deliver EVERY dac's gained output to the device, summed — multiple
    // dacs sum-mix like Pd (mono duplicates to both ears; stereo adds
    // interleaved).
    for (const auto& dac : dacs_)
        if (dac.desc->output_ptr)
            if (auto* a = static_cast<const AudioBuffer*>(
                    dac.desc->output_ptr(dac.data, "audio_out"))) {
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
        if (!r->from.desc->output_ptr) continue;
        auto* a = static_cast<const AudioBuffer*>(
            r->from.desc->output_ptr(r->from.data, r->port.c_str()));
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
        if (auto v = read_output(s->from, s->edge->from_port, "scalar"))
            if (auto* d = std::get_if<double>(&*v))
                s->value.store(*d, std::memory_order_relaxed);
    for (auto& q : queues_)
        if (!q->into_block)
            if (auto v = read_output(q->from, q->edge->from_port, q->kind))
                if (auto* d = std::get_if<double>(&*v); d && *d > 0.5)
                    q->pending.fetch_add(1, std::memory_order_acq_rel);
}

void AudioRegion::publish(Graph&) {
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
        if (r->slot)
            *r->slot = AudioBuffer{r->drained.data(), frames, ch,
                                   r->sample_rate};
    }
    for (auto& s : snaps_)
        apply_value(s->to, s->edge->to_port.c_str(),
                    PortValue{s->value.load(std::memory_order_relaxed)});
    for (auto& q : queues_) {
        if (q->into_block) continue;
        if (q->pending.load(std::memory_order_acquire) > 0) {
            q->pending.fetch_sub(1, std::memory_order_acq_rel);
            apply_value(q->to, q->edge->to_port.c_str(), PortValue{1.0});
            q->applied_high = true;
        } else if (q->applied_high) {
            apply_value(q->to, q->edge->to_port.c_str(), PortValue{0.0});
            q->applied_high = false;
        }
    }
}

void AudioRegion::capture_latches(const Graph&) {
    for (auto& q : queues_) {
        if (!q->into_block) continue;
        if (auto v = read_output(q->from, q->edge->from_port, q->kind))
            if (auto* d = std::get_if<double>(&*v); d && *d > 0.5)
                q->pending.fetch_add(1, std::memory_order_acq_rel);
    }
    for (auto& l : latches_) {
        auto v = read_output(l->from, l->edge->from_port, l->kind);
        if (!v) continue;
        std::lock_guard<std::mutex> lock(l->m);
        l->value = std::move(*v);
        l->set   = true;
    }
}

void AudioRegion::pump_offline(double dt) {
    // Flush the callback's debug snapshot from a thread that may log.
    if (dbg_ready_.load(std::memory_order_acquire)) {
        BLKLOG("%s", dbg_buf_);
        dbg_ready_.store(false, std::memory_order_release);
    }
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
