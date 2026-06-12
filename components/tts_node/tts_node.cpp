// Copyright 2026 Travis West
#include "tts_node.hpp"
#include "worker.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>

void TtsNode::operator()(double) {
    bool seq_changed = inputs.seq.value != prev_seq_;
    prev_seq_        = inputs.seq.value;
    if (inputs.say.triggered || seq_changed) {
        std::string msg = inputs.message.value;
        std::string cmd = inputs.command.value.empty()
            ? "companion/.venv-melotts/bin/python companion/tts_cli.py"
            : inputs.command.value;
        std::string url = inputs.play_url.value.empty()
            ? "http://127.0.0.1:8080/play" : inputs.play_url.value;
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
    outputs.spoken.value = sh_->spoken.exchange(false) ? 1.f : 0.f;
}
