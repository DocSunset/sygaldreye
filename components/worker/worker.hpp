// Copyright 2026 Travis West
#pragma once
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

// The worker region (edge_executor.design.md step 4): one background thread
// per peer where jobs MAY block — system(), sleeps, network. Nodes post
// jobs from their tick instead of stalling the render region; results flow
// back through atomics/shared state owned by the poster. Jobs run in
// posting order, never dropped (this is a queue mapping with a thread on
// the consuming end).
class Worker {
public:
    static Worker& shared() {
        static Worker w;
        return w;
    }

    void post(std::function<void()> job) {
        {
            std::lock_guard<std::mutex> lock(m_);
            q_.push_back(std::move(job));
        }
        cv_.notify_one();
    }

    ~Worker() {
        {
            std::lock_guard<std::mutex> lock(m_);
            stop_ = true;
        }
        cv_.notify_one();
        thread_.join();
    }

private:
    Worker() : thread_([this] { run(); }) {}
    void run() {
        std::unique_lock<std::mutex> lock(m_);
        while (true) {
            cv_.wait(lock, [this] { return stop_ || !q_.empty(); });
            if (q_.empty()) return;  // stop requested and drained
            auto job = std::move(q_.front());
            q_.pop_front();
            lock.unlock();
            job();
            lock.lock();
        }
    }

    std::mutex                        m_;
    std::condition_variable           cv_;
    std::deque<std::function<void()>> q_;
    bool                              stop_ = false;
    std::thread                       thread_;
};
