// Copyright 2026 Travis West
#pragma once
#include <deque>
#include <mutex>
#include <utility>

// The canonical `queue` mapping (planning/edge_executor.design.md): events
// crossing thread boundaries — MPSC, never drops, never duplicates.
// Producers push from any thread; one consumer drains at its own cadence.
// Replaces every hand-rolled pending/take pattern (take_edit, param queues).
template<typename T>
class EventQueue {
public:
    void push(T v) {
        std::lock_guard<std::mutex> lock(m_);
        q_.push_back(std::move(v));
    }
    // All pending events, in arrival order; queue is left empty.
    std::deque<T> drain() {
        std::lock_guard<std::mutex> lock(m_);
        return std::exchange(q_, {});
    }
    bool empty() const {
        std::lock_guard<std::mutex> lock(m_);
        return q_.empty();
    }

private:
    mutable std::mutex m_;
    std::deque<T>      q_;
};
