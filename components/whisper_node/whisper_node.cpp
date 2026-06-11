// Copyright 2026 Travis West
#include "whisper_node.hpp"
#include "worker.hpp"
#include <array>
#include <cstdio>
#include <fstream>

namespace {
std::string json_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c == '\n') out += "\\n";
        else if (static_cast<unsigned char>(c) >= 0x20) out += c;
    }
    return out;
}
} // namespace

void WhisperNode::operator()(double) {
    if (inputs.seq.value != prev_seq_) {
        prev_seq_ = inputs.seq.value;
        std::string wav = inputs.file.value;
        std::string cmd = inputs.command.value.empty()
            ? "companion/.venv/bin/python companion/transcribe_cli.py"
            : inputs.command.value;
        std::string url = inputs.target_url.value.empty()
            ? "http://127.0.0.1:8930/param" : inputs.target_url.value;
        std::string node = inputs.target_node.value.empty()
            ? "claude" : inputs.target_node.value;
        auto sh = sh_;
        Worker::shared().post([wav, cmd, url, node, sh] {
            if (wav.empty()) return;
            std::array<char, 4096> buf{};
            std::string transcript;
            FILE* p = popen((cmd + " " + wav + " 2>/dev/null").c_str(), "r");
            if (!p) return;
            while (std::fgets(buf.data(), int(buf.size()), p))
                transcript += buf.data();
            if (pclose(p) != 0 || transcript.empty()) {
                std::fprintf(stderr, "whisper_node: transcription failed\n");
                return;
            }
            while (!transcript.empty() && transcript.back() == '\n')
                transcript.pop_back();

            char body_path[] = "/tmp/whisper_node_fwd.json";
            {
                std::ofstream f(body_path);
                f << R"({"node":")" << node << R"(","params":{"message":")"
                  << json_escape(transcript) << R"(","seq":)" << ++sh->fwd_seq
                  << "}}";
            }
            if (std::system(("curl -sf -X POST --data-binary @" +
                             std::string(body_path) + " " + url + " >/dev/null")
                                .c_str()) != 0) {
                std::fprintf(stderr, "whisper_node: forward to %s failed\n", url.c_str());
                return;
            }
            sh->sent = true;
        });
    }
    outputs.sent.value = sh_->sent.exchange(false) ? 1.f : 0.f;
}
