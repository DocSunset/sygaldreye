// Copyright 2025 Travis West
#include "hrtf_spatializer.hpp"

#include <algorithm>
#include <cmath>

static constexpr float kSpeedOfSound = 343.0f;

HrtfSpatializer::HrtfSpatializer(SpatializerParams const& p) : params_(p) {
    PositionState zero{};
    update_coeffs(zero);
}

void HrtfSpatializer::set_position(Eigen::Vector3f direction, float distance_m) {
    // Source at the listener's head: normalize(0) is 0/0 = NaN, which
    // permanently poisons the IIR state (silent spatialize, 2026-06-12).
    // Treat it as dead ahead at minimum distance instead.
    float len = direction.norm();
    direction = (len > 1e-6f) ? Eigen::Vector3f(direction / len) : Eigen::Vector3f(0.f, 0.f, -1.f);
    PositionState s;
    s.azimuth = std::asin(std::clamp(direction.x(), -1.0f, 1.0f));
    s.elevation = std::asin(std::clamp(direction.y(), -1.0f, 1.0f));
    s.distance_m = distance_m;

    // Write into the write slot, then atomically publish it as the new read slot.
    // The old read slot becomes the new write slot (triple-buffer rotation).
    int w = write_idx_.load(std::memory_order_relaxed);
    pos_buf_[static_cast<size_t>(w)] = s;
    int old_read = read_idx_.exchange(w, std::memory_order_release);
    write_idx_.store(old_read, std::memory_order_relaxed);
}

void HrtfSpatializer::update_coeffs(PositionState const& p) {
    float sr = params_.sample_rate;

    // ILD: low-shelf on contralateral, gain proportional to azimuth shadow
    float shelf_gain_db = -6.0f * std::abs(std::sin(p.azimuth));
    shelf_filter_.set_coeffs(synth::low_shelf(1000.0f, shelf_gain_db, sr));

    // Air absorption LP: cutoff from ~20 kHz at 0 m to 2000 Hz at 20 m
    float dist_clamped = std::max(p.distance_m, 0.1f);
    float cutoff = std::max(2000.0f, 20000.0f - 900.0f * dist_clamped);
    absorb_filter_.set_coeffs(synth::low_pass(cutoff, 0.707f, sr));

    // Pinna notch: centre frequency 8–12 kHz mapped by elevation [-π/2, π/2]
    float notch_freq = 8000.0f + 4000.0f * (p.elevation / (3.14159265f * 0.5f) * 0.5f + 0.5f);
    auto nc = synth::notch(notch_freq, 8.0f, sr);
    notch_l_.set_coeffs(nc);
    notch_r_.set_coeffs(nc);
}

float HrtfSpatializer::read_delayed(float delay_samples) const {
    int d0 = static_cast<int>(delay_samples);
    float frac = delay_samples - static_cast<float>(d0);
    d0 = std::min(d0, kDelayBufSize - 2);
    int i0 = (delay_write_ - 1 - d0 + kDelayBufSize) % kDelayBufSize;
    int i1 = (i0 - 1 + kDelayBufSize) % kDelayBufSize;
    return delay_buf_[static_cast<size_t>(i0)] * (1.0f - frac) +
           delay_buf_[static_cast<size_t>(i1)] * frac;
}

void HrtfSpatializer::process(float const* mono_in, float* stereo_out, int frames) {
    // A NaN that slips into IIR feedback or the delay line never leaves.
    // (NaN + x = NaN, so one summed check covers all four filters.)
    if (!std::isfinite(
            shelf_filter_.s1 + absorb_filter_.s1 + notch_l_.s1 + notch_r_.s1 + delay_buf_[0])) {
        shelf_filter_.s1 = shelf_filter_.s2 = 0.f;
        absorb_filter_.s1 = absorb_filter_.s2 = 0.f;
        notch_l_.s1 = notch_l_.s2 = 0.f;
        notch_r_.s1 = notch_r_.s2 = 0.f;
        delay_buf_.fill(0.f);
    }

    PositionState pos = pos_buf_[static_cast<size_t>(read_idx_.load(std::memory_order_acquire))];

    if (pos.azimuth != prev_azimuth_ || pos.elevation != prev_elevation_ ||
        pos.distance_m != prev_distance_) {
        update_coeffs(pos);
        prev_azimuth_ = pos.azimuth;
        prev_elevation_ = pos.elevation;
        prev_distance_ = pos.distance_m;
    }

    // 1 m reference distance: closer than 1 m never amplifies — a source
    // clamped to the listener's head was 10x gain and clipped the dac.
    float dist_gain = 1.0f / std::max(pos.distance_m, 1.0f);
    float delay_samples = (params_.head_radius_m / kSpeedOfSound) * params_.sample_rate *
                          (pos.azimuth + std::sin(pos.azimuth));

    // Positive azimuth = source right → left ear is contralateral (delayed)
    bool source_right = pos.azimuth > 0.0f;

    for (int i = 0; i < frames; ++i) {
        float in = mono_in[i] * dist_gain;

        delay_buf_[static_cast<size_t>(delay_write_)] = in;
        delay_write_ = (delay_write_ + 1) % kDelayBufSize;

        float direct = in;
        float delayed = read_delayed(std::abs(delay_samples));
        float attenuated = shelf_filter_.tick(delayed);
        attenuated = absorb_filter_.tick(attenuated);

        float left, right;
        if (source_right) {
            right = direct;
            left = attenuated;
        } else {
            left = direct;
            right = attenuated;
        }

        left = notch_l_.tick(left);
        right = notch_r_.tick(right);

        stereo_out[i] += left;  // planar: [L block][R block]
        stereo_out[frames + i] += right;
    }
}
