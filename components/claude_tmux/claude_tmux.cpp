// Copyright 2026 Travis West
#include "claude_tmux.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>

namespace {
std::string q(const std::string& s) {  // single-quote for shell
    std::string out = "'";
    for (char c : s) { if (c == '\'') out += "'\\''"; else out += c; }
    return out + "'";
}
} // namespace

bool ClaudeTmuxNode::ensure_session() {
    just_spawned_ = false;
    std::string ses = inputs.session.value.empty() ? "claude_vr" : inputs.session.value;
    if (std::system(("tmux has-session -t " + q(ses) + " 2>/dev/null").c_str()) == 0) {
        running_ = true;
        return true;
    }
    // --settings marks the hook config as user-provided so the Stop hook
    // (speak replies) doesn't wait on interactive approval.
    std::string cmd = inputs.command.value.empty()
        ? "claude --settings voice_hooks.json" : inputs.command.value;
    std::string dir = inputs.workdir.value.empty() ? "companion/claude_vr"
                                                   : inputs.workdir.value;
    int rc = std::system(("tmux new-session -d -s " + q(ses) +
                          " -c " + q(dir) + " " + q(cmd)).c_str());
    running_      = (rc == 0);
    just_spawned_ = running_;
    if (!running_) std::fprintf(stderr, "claude_tmux: failed to start session\n");
    return running_;
}

void ClaudeTmuxNode::deliver(const std::string& msg) {
    if (msg.empty()) return;
    if (!ensure_session()) return;
    if (just_spawned_) {  // fresh spawn: the TUI eats keys while booting
        pending_msg_   = msg;
        warmup_ticks_  = 240;  // ~4 s at 60 fps
        return;
    }
    std::string ses = inputs.session.value.empty() ? "claude_vr" : inputs.session.value;
    // load-buffer + paste-buffer sidesteps shell/tmux quoting of the message
    std::string tmp = "/tmp/claude_tmux_msg.txt";
    { std::ofstream f(tmp); f << msg; }
    int rc = std::system(("tmux load-buffer -b cvr " + tmp +
                          " && tmux paste-buffer -d -b cvr -t " + q(ses) +
                          " && tmux send-keys -t " + q(ses) + " Enter").c_str());
    if (rc != 0) { std::fprintf(stderr, "claude_tmux: deliver failed\n"); return; }
    outputs.sent.value = 1.f;
}

void ClaudeTmuxNode::operator()(double) {
    outputs.sent.value = 0.f;
    bool send_now = inputs.send.value > 0.5f;
    bool edge     = send_now && !prev_send_;
    prev_send_    = send_now;
    bool seq_changed = inputs.seq.value != prev_seq_;
    prev_seq_        = inputs.seq.value;

    if (warmup_ticks_ > 0 && --warmup_ticks_ == 0 && !pending_msg_.empty()) {
        std::string m = std::move(pending_msg_);
        pending_msg_.clear();
        deliver(m);
    }
    if (edge || seq_changed) deliver(inputs.message.value);
    outputs.running.value = running_ ? 1.f : 0.f;
}
