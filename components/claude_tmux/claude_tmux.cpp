// Copyright 2026 Travis West
#include "claude_tmux.hpp"
#include "worker.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <thread>

namespace {
std::string q(const std::string& s) {  // single-quote for shell
    std::string out = "'";
    for (char c : s) { if (c == '\'') out += "'\\''"; else out += c; }
    return out + "'";
}
} // namespace

void ClaudeTmuxNode::operator()(double) {
    bool send_now = endpoints.send.get() > 0.5f;
    bool edge     = send_now && !prev_send_;
    prev_send_    = send_now;
    bool seq_changed = endpoints.seq.get() != prev_seq_;
    prev_seq_        = endpoints.seq.get();

    if (edge || seq_changed) {
        std::string ses = endpoints.session.get().empty() ? "claude_vr" : endpoints.session.get();
        std::string cmd = endpoints.command.get().empty()
            ? "claude --settings voice_hooks.json" : endpoints.command.get();
        std::string dir = endpoints.workdir.get().empty() ? "companion/claude_vr"
                                                       : endpoints.workdir.get();
        std::string msg = endpoints.message.get();
        auto sh = sh_;
        Worker::shared().post([ses, cmd, dir, msg, sh] {
            if (msg.empty()) return;
            bool fresh = false;
            if (std::system(("tmux has-session -t " + q(ses) + " 2>/dev/null").c_str()) != 0) {
                // --settings marks the hook config as user-provided so the
                // Stop hook (speak replies) skips interactive approval.
                if (std::system(("tmux new-session -d -s " + q(ses) +
                                 " -c " + q(dir) + " " + q(cmd)).c_str()) != 0) {
                    std::fprintf(stderr, "claude_tmux: failed to start session\n");
                    sh->running = false;
                    return;
                }
                fresh = true;
            }
            sh->running = true;
            // Fresh TUI eats keys while booting; worker may simply wait.
            if (fresh) std::this_thread::sleep_for(std::chrono::seconds(4));

            // load-buffer + paste-buffer sidesteps shell/tmux quoting; the
            // TUI swallows an Enter that lands mid-paste — breathe between.
            std::string tmp = "/tmp/claude_tmux_msg.txt";
            { std::ofstream f(tmp); f << msg; }
            int rc = std::system(("tmux load-buffer -b cvr " + tmp +
                                  " && tmux paste-buffer -d -b cvr -t " + q(ses) +
                                  " && sleep 0.4 && tmux send-keys -t " + q(ses) + " Enter").c_str());
            if (rc != 0) { std::fprintf(stderr, "claude_tmux: deliver failed\n"); return; }
            sh->sent = true;
        });
    }

    endpoints.sent.value    = sh_->sent.exchange(false) ? 1.f : 0.f;
    endpoints.running.value = sh_->running.load() ? 1.f : 0.f;
}
