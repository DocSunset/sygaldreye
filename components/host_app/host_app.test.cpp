// Copyright 2026 Travis West
// Every shipped graph must parse against the host registry — a broken
// assets/graphs/*.json fails here instead of at headset boot.
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

#include "host_app.hpp"
#include "signal_graph.hpp"

TEST(ShippedGraphs, EveryAssetGraphParses) {
    ComponentRegistry reg;
    register_host_nodes(reg);
    int checked = 0;
    for (const auto& e : std::filesystem::directory_iterator(ASSETS_GRAPHS_DIR)) {
        if (e.path().extension() != ".json") continue;
        std::ifstream f(e.path());
        std::stringstream ss;
        ss << f.rdbuf();
        auto g = parse_graph(ss.str(), reg);
        EXPECT_NE(g, nullptr) << e.path().filename() << " failed to parse";
        ++checked;
    }
    EXPECT_GT(checked, 0) << "no graphs found under " ASSETS_GRAPHS_DIR;
}
