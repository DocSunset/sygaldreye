// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <atomic>
#include <memory>
#include <string>
#include <string_view>

// Owns a tmux session running a coding-assistant CLI and exposes an input
// for dumping text into it. Voice flow: companion transcribes speech and
// POSTs /param {"node":"claude","params":{"message":"...","seq":N}} — the
// node fires whenever seq changes (params persist, so plain triggers can't
// re-fire). The session's workdir carries a Claude Code Stop hook that
// speaks every reply via the companion's /tts.
//
// Deliveries run on the worker region: tmux spawn/paste (and the deliberate
// sleeps around a freshly-booted TUI) block the worker thread, never the
// render thread. Results flow back through a shared atomic block that
// outlives this instance (jobs may still be queued when a graph swap
// destroys it).
struct ClaudeTmuxNode {
    static consteval std::string_view name() { return "claude_tmux"; }
    static consteval std::string_view source_header() { return "components/claude_tmux/claude_tmux.hpp"; }

    struct inputs {
        ::text<"session">  session;   // empty → "claude_vr"
        ::text<"command">  command;   // empty → "claude"
        ::text<"workdir">  workdir;   // empty → "companion/claude_vr"
        ::text<"message">  message;
        slider<"seq",  "", float, fp(0.f), fp(1e9f), fp(0.f)> seq;
        slider<"send", "", float, fp(0.f), fp(1.f),  fp(0.f)> send;  // manual edge
    } inputs;

    struct outputs {
        port<"running", float> running;
        port<"sent",    float> sent;  // pulses 1 on the tick a delivery lands
    } outputs;

    void operator()(double);

private:
    struct Shared {
        std::atomic_bool running{false};
        std::atomic_bool sent{false};
    };
    std::shared_ptr<Shared> sh_ = std::make_shared<Shared>();
    float prev_seq_  = 0.f;
    bool  prev_send_ = false;
};
