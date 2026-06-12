// Copyright 2026 Travis West
#include "tts_local.hpp"
#include "worker.hpp"
#include "sherpa-onnx/c-api/c-api.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>

namespace {
// model.onnx, else the first *.onnx in the directory (piper voices ship
// as <voice-name>.onnx).
std::string find_model(const std::string& dir) {
    std::string preferred = dir + "/model.onnx";
    if (std::ifstream(preferred).good()) return preferred;
    if (DIR* d = opendir(dir.c_str())) {
        while (dirent* e = readdir(d)) {
            std::string_view n{e->d_name};
            if (n.size() > 5 && n.substr(n.size() - 5) == ".onnx") {
                std::string found = dir + "/" + std::string(n);
                closedir(d);
                return found;
            }
        }
        closedir(d);
    }
    return preferred;
}
} // namespace

TtsLocalNode::~TtsLocalNode() {
    auto sh = sh_;
    Worker::shared().post([sh] {
        std::lock_guard<std::mutex> lock(sh->m);
        if (sh->tts) { SherpaOnnxDestroyOfflineTts(sh->tts); sh->tts = nullptr; }
    });
}

void TtsLocalNode::operator()(double t) {
    bool seq_changed = inputs.seq.value != prev_seq_;
    prev_seq_        = inputs.seq.value;
    if (inputs.say.triggered || seq_changed) {
        std::string msg   = inputs.message.value;
        std::string dir   = inputs.model_dir.value.empty()
            ? "assets/models/vits-piper-en_US-ryan-medium"
            : inputs.model_dir.value;
        float speed = inputs.speed.value;
        int   sid   = int(inputs.sid.value);
        auto  sh    = sh_;
        Worker::shared().post([sh, msg, dir, speed, sid] {
            if (msg.empty()) return;
            {
                std::lock_guard<std::mutex> lock(sh->m);
                if (!sh->tts) {
                    std::string model  = find_model(dir);
                    std::string tokens = dir + "/tokens.txt";
                    std::string data   = dir + "/espeak-ng-data";
                    SherpaOnnxOfflineTtsConfig cfg;
                    std::memset(&cfg, 0, sizeof(cfg));
                    cfg.model.vits.model    = model.c_str();
                    cfg.model.vits.tokens   = tokens.c_str();
                    cfg.model.vits.data_dir = data.c_str();
                    cfg.model.num_threads   = 2;
                    cfg.model.provider      = "cpu";
                    sh->tts = SherpaOnnxCreateOfflineTts(&cfg);
                    if (!sh->tts)
                        std::fprintf(stderr, "tts_local: failed to load %s\n",
                                     dir.c_str());
                }
            }
            if (!sh->tts) return;
            const SherpaOnnxGeneratedAudio* a =
                SherpaOnnxOfflineTtsGenerate(sh->tts, msg.c_str(), sid, speed);
            if (!a) return;
            // Linear resample model rate → 48 kHz engine rate.
            std::vector<float> w(std::size_t(a->n) * 48000 /
                                 std::size_t(std::max(1, a->sample_rate)));
            for (std::size_t i = 0; i < w.size(); ++i) {
                double s  = double(i) * a->sample_rate / 48000.0;
                auto   i0 = std::size_t(s);
                auto   i1 = std::min(i0 + 1, std::size_t(a->n - 1));
                float  fr = float(s - double(i0));
                w[i] = a->samples[i0] * (1.f - fr) + a->samples[i1] * fr;
            }
            SherpaOnnxDestroyOfflineTtsGeneratedAudio(a);
            std::lock_guard<std::mutex> lock(sh->m);
            sh->pending.insert(sh->pending.end(), w.begin(), w.end());
        });
    }

    // Pace queued speech out as an ordinary audio edge.
    double dt = (prev_t_ > 0.0) ? t - prev_t_ : 1.0 / 60.0;
    prev_t_ = t;
    if (dt <= 0.0 || dt > 0.1) dt = 1.0 / 60.0;
    int n = std::clamp(int(dt * 48000.0), 0, 4800);

    if (queue_pos_ >= queue_.size()) {
        queue_.clear();
        queue_pos_ = 0;
        std::lock_guard<std::mutex> lock(sh_->m);
        std::swap(queue_, sh_->pending);
    }
    buf_.assign(std::size_t(n), 0.f);
    int got = 0;
    for (; got < n && queue_pos_ < queue_.size(); ++got)
        buf_[std::size_t(got)] = queue_[queue_pos_++];
    outputs.audio.value    = AudioBuffer{buf_.data(), n, 1, 48000};
    outputs.speaking.value = (queue_pos_ < queue_.size()) ? 1.f : 0.f;
}
