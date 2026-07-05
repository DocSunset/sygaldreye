// Copyright 2026 Travis West
#include "event_queue.hpp"
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

TEST(EventQueue, DrainsInOrderAndEmpties) {
    EventQueue<std::string> q;
    q.push("a");
    q.push("b");
    auto got = q.drain();
    ASSERT_EQ(got.size(), 2u);
    EXPECT_EQ(got[0], "a");
    EXPECT_EQ(got[1], "b");
    EXPECT_TRUE(q.empty());
}

TEST(EventQueue, NeverDropsAcrossProducers) {
    EventQueue<int> q;
    constexpr int kPerThread = 1000, kThreads = 4;
    std::vector<std::thread> producers;
    for (int t = 0; t < kThreads; ++t)
        producers.emplace_back([&q, t] {
            for (int i = 0; i < kPerThread; ++i) q.push(t * kPerThread + i);
        });
    for (auto& p : producers) p.join();

    std::vector<char> seen(kThreads * kPerThread, 0);
    for (int v : q.drain()) seen[static_cast<size_t>(v)] = 1;
    for (char s : seen) EXPECT_TRUE(s);  // every event exactly once
}
