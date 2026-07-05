// Copyright 2025 Travis West
#pragma once
#include "signal_graph.hpp"
#include <cstddef>
#include <vector>

std::vector<std::size_t> topo_sort(const std::vector<NodeInstance>& nodes,
                                   const std::vector<Edge>& edges);
