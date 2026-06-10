// Copyright 2026 Travis West
#pragma once
#include <memory>

// Owning-pointer support for SubgraphDescriptor without the complete type.
// subgraph_node.hpp includes signal_graph.hpp, so headers underneath it
// (signal_graph, component_registry) must not include it back; the
// out-of-line deleter keeps their unique_ptrs free of sizeof(T).
struct SubgraphDescriptor;
struct SubgraphDescriptorDeleter {
    void operator()(SubgraphDescriptor*) const;
};
using SubgraphDescriptorPtr = std::unique_ptr<SubgraphDescriptor, SubgraphDescriptorDeleter>;
