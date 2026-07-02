// Copyright 2026 Travis West
#include "claude_tmux.hpp"

#include <gtest/gtest.h>

#include "eyeballs_node_abi.hpp"

// M5: the node owns a tmux session — lifting it would spawn N sessions.
TEST(ClaudeTmux, IsResourceHolder) {
    const EyeballsNodeDescriptor* d = make_descriptor<ClaudeTmuxNode>();
    EXPECT_EQ(d->lift_kind, EYEBALLS_LIFT_RESOURCE_HOLDER);
}
