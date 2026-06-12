// Copyright 2026 Travis West
#pragma once
#include "audio_output.hpp"
#include <atomic>
#include <cstddef>
#include <mutex>
#include <vector>

// THE single owner of audio hardware (ratified: the Pd model). One
// process, one engine, one stable output+input stream pair — streams are
// NEVER churned by graph edits; node lifetimes own no hardware. dac and
// adc nodes are taps: every dac's output is summed into the callback by
// the block scheduler; every adc reads the engine's shared input ring.
// System sounds (/play) mix in at the engine, independent of the graph.
class AudioEngine {
public:
    static AudioEngine& instance();

    // The block scheduler registers its render once (stable owner).
    // The engine opens the output stream on first ensure() and keeps it.
    void ensure_output(AudioCallback render);
    bool has_device() const { return output_.has_value() && !output_->dead(); }
    // Recreate a disconnected stream (call from a non-audio thread).
    void recover_if_dead();

    // Input taps (adc/mic nodes): refcounted; the capture stream opens on
    // the first tap. read_input drains the shared SPSC ring (any single
    // consumer thread; mono float).
    void add_input_tap();
    void remove_input_tap();
    int  read_input(float* dst, int max_frames);

    // System sounds: queue a mono wav (engine-rate) mixed into the output
    // callback after the graph — /play works on any graph, even empty.
    void play(std::vector<float> samples);

    bool enable_device = true;  // tests/headless force silence + no streams

    static constexpr int kRate = 48000;

private:
    AudioEngine() = default;
    void open_input_locked();
    void callback(float* out, int frames);  // audio thread

    std::mutex                 setup_mutex_;
    std::optional<AudioOutput> output_;
    AudioCallback              render_;

    // input (guarded by setup_mutex_ for open/close; ring is SPSC)
    struct InputState;
    InputState* input_ = nullptr;
    int         input_taps_ = 0;

    // system sound slots (small, swapped under mutex, mixed lock-free-ish)
    std::mutex                      play_mutex_;
    std::vector<std::vector<float>> play_queue_;
    std::vector<float>              playing_;
    std::size_t                     play_pos_ = 0;
};
