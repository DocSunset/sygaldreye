// Copyright 2026 Travis West
#include "tts_node.hpp"
#include "worker.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>

void TtsNode::operator()(double) {
    bool seq_changed = endpoints.seq.get() != prev_seq_;
    prev_seq_        = endpoints.seq.get();
    bool fire = endpoints.say.triggered || seq_changed;
    endpoints.say.triggered = false;   // events are deliveries
    if (fire) {
        std::string msg = endpoints.message.get();
        std::string cmd = endpoints.command.get().empty()
            ? "companion/.venv-melotts/bin/python companion/tts_cli.py"
            : endpoints.command.get();
        std::string url = endpoints.play_url.get().empty()
            ? "http://127.0.0.1:8080/play" : endpoints.play_url.get();
        auto sh = sh_;
        Worker::shared().post([msg, cmd, url, sh] {
            if (msg.empty()) return;
            const char* txt = "/tmp/tts_node_msg.txt";
            const char* wav = "/tmp/tts_node_out.wav";
            { std::ofstream f(txt); f << msg; }
            std::remove(wav);
            if (std::system((cmd + " " + txt + " " + wav).c_str()) != 0) {
                std::fprintf(stderr, "tts_node: synthesis failed\n");
                return;
            }
            if (std::system(("curl -sf -X POST --data-binary @" + std::string(wav) +
                             " " + url + " >/dev/null").c_str()) != 0) {
                std::fprintf(stderr, "tts_node: upload to %s failed\n", url.c_str());
                return;
            }
            sh->spoken = true;
        });
    }
    endpoints.spoken.value = sh_->spoken.exchange(false) ? 1.f : 0.f;
}
