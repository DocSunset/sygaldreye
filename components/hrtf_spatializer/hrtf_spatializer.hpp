#pragma once
#include <biquad_filter.hpp>
#include <Eigen/Core>
#include <atomic>
#include <array>

struct SpatializerParams {
    float head_radius_m = 0.0875f;
    float sample_rate   = 48000.0f;
};

class HrtfSpatializer {
public:
    explicit HrtfSpatializer(SpatializerParams const& = {});
    void set_position(Eigen::Vector3f direction, float distance_m);
    void process(float const* mono_in, float* stereo_out, int frames);

private:
    struct PositionState {
        float azimuth    = 0.0f;
        float elevation  = 0.0f;
        float distance_m = 1.0f;
    };

    SpatializerParams params_;

    // Triple-buffer for lock-free scene->audio position updates
    std::array<PositionState, 3> pos_buf_{};
    std::atomic<int> write_idx_{1};
    std::atomic<int> read_idx_{0};

    // Fractional delay line for contralateral ITD
    static constexpr int kDelayBufSize = 64;
    std::array<float, kDelayBufSize> delay_buf_{};
    int delay_write_ = 0;

    // Filters
    synth::BiquadFilter shelf_filter_;    // ILD low-shelf on contralateral
    synth::BiquadFilter absorb_filter_;   // air absorption LP
    synth::BiquadFilter notch_l_;         // pinna notch, left
    synth::BiquadFilter notch_r_;         // pinna notch, right

    // Cached state to detect when coefficients need updating
    float prev_azimuth_   = 1e9f;
    float prev_elevation_ = 1e9f;
    float prev_distance_  = 1e9f;

    void update_coeffs(PositionState const& p);
    float read_delayed(float delay_samples) const;
};
