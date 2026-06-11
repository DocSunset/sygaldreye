// Copyright 2026 Travis West
#include "audio_region.hpp"
#include "signal_graph_sort.hpp"
#include "port_schema_reader.hpp"
#include <algorithm>
#include <cstring>

namespace {
constexpr int    kRate     = 48000;
constexpr size_t kRingCap  = 48000 * 2;  // 2 s per crossing edge

// Kind of a producer's output port, from its schema.
std::string out_kind(const NodeInstance& n, const std::string& port) {
    PortSchema s = parse_port_schema(n.desc ? n.desc->port_schema : nullptr);
    for (const auto& p : s.outputs)
        if (p.name == port) return p.kind;
    return "unknown";
}
} // namespace

void AudioRegion::rebuild(Graph& g) {
    std::lock_guard<std::mutex> lock(plan_mutex_);
    order_.clear();
    internal_.clear();
    latches_.clear();
    rings_.clear();
    snaps_.clear();
    g.offrender.clear();
    dac_out_key_.clear();

    std::unordered_map<std::string, const NodeInstance*> by_id;
    for (auto& n : g.nodes) by_id[n.id] = &n;

    // Region membership: dacs + upstream closure through audio edges.
    std::unordered_set<std::string> region;
    std::vector<std::string> frontier;
    for (auto& n : g.nodes)
        if (std::string_view{n.desc->type_name} == "dac") {
            region.insert(n.id);
            frontier.push_back(n.id);
            if (dac_out_key_.empty()) dac_out_key_ = n.id + ".out";
        }
    while (!frontier.empty()) {
        std::string id = std::move(frontier.back());
        frontier.pop_back();
        for (const auto& e : g.edges) {
            if (e.to_node != id || region.count(e.from_node)) continue;
            auto it = by_id.find(e.from_node);
            if (it == by_id.end()) continue;
            if (out_kind(*it->second, e.from_port) != "audio") continue;
            region.insert(e.from_node);
            frontier.push_back(e.from_node);
        }
    }
    if (region.empty()) {
        stream_.reset();
        return;
    }

    for (const auto& id : region) g.offrender.insert(id);

    std::vector<NodeInstance> sub;
    for (auto& n : g.nodes)
        if (region.count(n.id)) sub.push_back(n);

    for (const auto& e : g.edges) {
        bool from_in = region.count(e.from_node), to_in = region.count(e.to_node);
        if (from_in && to_in) {
            internal_.push_back(e);
        } else if (!from_in && to_in) {
            auto it = by_id.find(e.to_node);
            if (it == by_id.end()) continue;
            auto l = std::make_unique<Latch>();
            l->from_key = e.from_node + "." + e.from_port;
            l->to       = *it->second;
            l->to_port  = e.to_port;
            latches_.push_back(std::move(l));
        } else if (from_in && !to_in) {
            auto it = by_id.find(e.from_node);
            if (it == by_id.end()) continue;
            if (out_kind(*it->second, e.from_port) == "audio") {
                auto r = std::make_unique<Ring>();
                r->from_key = e.from_node + "." + e.from_port;
                r->buf.resize(kRingCap);
                rings_.push_back(std::move(r));
            } else {
                auto s = std::make_unique<Snap>();
                s->from_key = e.from_node + "." + e.from_port;
                snaps_.push_back(std::move(s));
            }
        }
    }

    auto order_idx = topo_sort(sub, internal_);
    for (std::size_t i : order_idx) order_.push_back(sub[i]);

    if (!stream_ && enable_device) {
        stream_ = AudioOutput::create(
            [this](float* out, int frames) { render_block(out, frames); }, kRate);
        if (stream_) stream_->start();
    }
}

void AudioRegion::render_block(float* out, int frames) {
    std::memset(out, 0, std::size_t(frames) * 2 * sizeof(float));
    std::unique_lock<std::mutex> lock(plan_mutex_, std::try_to_lock);
    if (!lock.owns_lock() || order_.empty()) return;

    t_ += double(frames) / kRate;
    for (auto& l : latches_)
        if (l->set.load(std::memory_order_relaxed) && l->to.desc->set_scalar_in)
            l->to.desc->set_scalar_in(l->to.data, l->to_port.c_str(),
                                      l->value.load(std::memory_order_relaxed));

    for (auto& n : order_) {
        for (const auto& e : internal_) {
            if (e.to_node != n.id) continue;
            std::string key = e.from_node + "." + e.from_port;
            if (auto a = store_.find(key); a != store_.end()) {
                if (n.desc->set_audio_in)
                    n.desc->set_audio_in(n.data, e.to_port.c_str(),
                                         a->second.data, a->second.frames,
                                         a->second.channels, a->second.sample_rate);
            } else if (auto s = scalars_.find(key); s != scalars_.end()) {
                if (n.desc->set_scalar_in)
                    n.desc->set_scalar_in(n.data, e.to_port.c_str(), s->second);
            }
        }
        if (n.desc->process) n.desc->process(n.data, t_);
        if (n.desc->push_outputs) {
            EyeballsOutputCtx ctx{};
            ctx.store   = this;
            ctx.node_id = n.id.c_str();
            ctx.emit_scalar = [](void* store, const char* nid, const char* port, double v) {
                auto* r = static_cast<AudioRegion*>(store);
                r->scalars_[std::string(nid) + "." + port] = v;
            };
            ctx.emit_audio = [](void* store, const char* nid, const char* port,
                                const float* data, int fr, int ch, int rate) {
                auto* r = static_cast<AudioRegion*>(store);
                r->store_[std::string(nid) + "." + port] = AudioBuffer{data, fr, ch, rate};
            };
            n.desc->push_outputs(n.data, &ctx);
        }
    }

    // Deliver the dac's gained output to the device (mono → stereo).
    if (auto it = store_.find(dac_out_key_); it != store_.end()) {
        int n = std::min(frames, it->second.frames);
        for (int i = 0; i < n; ++i)
            out[i * 2] = out[i * 2 + 1] = it->second.data[i];
    }

    // Crossing mappings: ring pushes + scalar snapshots.
    for (auto& r : rings_) {
        auto it = store_.find(r->from_key);
        if (it == store_.end()) continue;
        r->sample_rate = it->second.sample_rate;
        size_t head = r->head.load(std::memory_order_relaxed);
        size_t tail = r->tail.load(std::memory_order_acquire);
        for (int i = 0; i < it->second.frames; ++i) {
            if (head - tail >= kRingCap) break;  // full: drop newest
            r->buf[head % kRingCap] = it->second.data[std::size_t(i) *
                                          std::size_t(std::max(1, it->second.channels))];
            ++head;
        }
        r->head.store(head, std::memory_order_release);
    }
    for (auto& s : snaps_)
        if (auto it = scalars_.find(s->from_key); it != scalars_.end())
            s->value.store(it->second, std::memory_order_relaxed);
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
        auto it = g.values.find(l->from_key);
        if (it == g.values.end() || !std::holds_alternative<double>(it->second))
            continue;
        l->value.store(std::get<double>(it->second), std::memory_order_relaxed);
        l->set.store(true, std::memory_order_relaxed);
    }
}

void AudioRegion::pump_offline(double dt) {
    if (stream_ || order_.empty()) return;
    int frames = std::clamp(int(dt * kRate), 0, 4800);
    if (frames == 0) return;
    static thread_local std::vector<float> buf;
    buf.resize(std::size_t(frames) * 2);
    render_block(buf.data(), frames);
}
