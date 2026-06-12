// Copyright 2026 Travis West
#include "stt_whisper.hpp"
#include "worker.hpp"
#include "whisper.h"
#include <algorithm>
#include <cstdio>

SttWhisperNode::~SttWhisperNode() {
    // The worker may still hold sh_; the context is freed exactly once
    // when the last owner lets go.
    auto sh = sh_;
    Worker::shared().post([sh] {
        std::lock_guard<std::mutex> lock(sh->m);
        if (sh->ctx) { whisper_free(sh->ctx); sh->ctx = nullptr; }
    });
}

void SttWhisperNode::operator()(double) {
    outputs.bip.value = 0.f;

    bool held = inputs.record.value > 0.5f;
    if (held && !recording_)  { recording_ = true;  outputs.bip.value = 1.f; }
    if (!held && recording_)  { recording_ = false; outputs.bip.value = 0.5f; }

    // Accumulate the take at 16 kHz (whisper's rate): average each
    // 3-sample group of the 48 kHz mic edge.
    const AudioBuffer& in = inputs.audio_in.value;
    if (recording_ && in.data && in.frames > 0) {
        for (int i = 0; i < in.frames; ++i) {
            ds_acc_ += in.data[i * std::max(1, in.channels)];
            if (++ds_phase_ == 3) {
                take_.push_back(ds_acc_ / 3.f);
                ds_acc_ = 0.f;
                ds_phase_ = 0;
            }
        }
    }

    bool send_now = inputs.send.value > 0.5f;
    if (send_now && !prev_send_ && !take_.empty()) {
        auto sh    = sh_;
        auto take  = std::make_shared<std::vector<float>>(std::move(take_));
        take_      = {};
        std::string model = inputs.model.value.empty()
            ? "assets/models/ggml-tiny.en.bin" : inputs.model.value;
        {
            std::lock_guard<std::mutex> lock(sh->m);
            sh->busy = true;
        }
        Worker::shared().post([sh, take, model] {
            {
                std::lock_guard<std::mutex> lock(sh->m);
                if (!sh->ctx) {
                    auto cp = whisper_context_default_params();
                    cp.use_gpu = false;
                    sh->ctx = whisper_init_from_file_with_params(model.c_str(), cp);
                    if (!sh->ctx)
                        std::fprintf(stderr, "stt_whisper: failed to load %s\n",
                                     model.c_str());
                }
            }
            std::string out;
            if (sh->ctx && take->size() > 16000 / 4) {  // ≥0.25 s
                auto wp = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
                wp.n_threads        = 4;
                wp.print_progress   = false;
                wp.print_realtime   = false;
                wp.no_timestamps    = true;
                // Whisper pads every take to a 30 s encoder window; shrink
                // the encoder context to the take's real length (measured
                // 77 s → target seconds for a 3 s take on Quest). +128
                // frames of headroom, floor 192 to stay coherent.
                wp.audio_ctx = std::clamp(
                    int(double(take->size()) / (16000.0 * 30.0) * 1500.0) + 128,
                    192, 1500);
                wp.temperature_inc = 0.0f;  // greedy only, no retry ladder
                if (whisper_full(sh->ctx, wp, take->data(),
                                 int(take->size())) == 0) {
                    int n = whisper_full_n_segments(sh->ctx);
                    for (int i = 0; i < n; ++i)
                        out += whisper_full_get_segment_text(sh->ctx, i);
                }
            }
            std::lock_guard<std::mutex> lock(sh->m);
            sh->text  = std::move(out);
            sh->fresh = true;
            sh->busy  = false;
        });
        recording_ = false;
        outputs.bip.value = 1.f;
    }
    prev_send_ = send_now;

    bool erase_now = inputs.erase.value > 0.5f;
    if (erase_now && !prev_erase_) {
        take_.clear();
        recording_ = false;
        outputs.bip.value = 0.5f;
    }
    prev_erase_ = erase_now;

    outputs.heard.triggered = false;
    {
        std::lock_guard<std::mutex> lock(sh_->m);
        outputs.busy.value = sh_->busy ? 1.f : 0.f;
        if (sh_->fresh) {
            outputs.text.value      = sh_->text;
            outputs.heard.triggered = true;
            sh_->fresh              = false;
        }
    }
    outputs.recording.value = recording_ ? 1.f : 0.f;
}
