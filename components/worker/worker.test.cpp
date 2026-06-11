// Copyright 2026 Travis West
#include "worker.hpp"
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <vector>

TEST(Worker, RunsJobsInOrderOffCaller) {
    std::vector<int> got;
    std::atomic_bool done{false};
    auto caller = std::this_thread::get_id();
    std::thread::id worker_tid;
    for (int i = 0; i < 3; ++i)
        Worker::shared().post([&, i] { got.push_back(i); });
    Worker::shared().post([&] {
        worker_tid = std::this_thread::get_id();
        done = true;
    });
    for (int i = 0; i < 200 && !done; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_TRUE(done.load());
    EXPECT_NE(worker_tid, caller);
    ASSERT_EQ(got.size(), 3u);
    EXPECT_EQ(got[0], 0);
    EXPECT_EQ(got[1], 1);
    EXPECT_EQ(got[2], 2);
}

TEST(Worker, BlockingJobDoesNotBlockPoster) {
    std::atomic_bool slow_done{false};
    auto t0 = std::chrono::steady_clock::now();
    Worker::shared().post([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        slow_done = true;
    });
    auto posted = std::chrono::steady_clock::now() - t0;
    EXPECT_LT(std::chrono::duration<double>(posted).count(), 0.05);
    for (int i = 0; i < 200 && !slow_done; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    EXPECT_TRUE(slow_done.load());
}
