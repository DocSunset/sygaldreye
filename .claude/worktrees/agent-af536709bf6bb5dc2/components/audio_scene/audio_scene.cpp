// Copyright 2025 Travis West
#include "audio_scene.hpp"
#include <cstring>
#include <new>

int AudioScene::add_source(AudioSource const& src)
{
    for (int i = 0; i < k_max_sources; ++i) {
        if (!slots_[i].active) {
            slots_[i].source = src;
            std::destroy_at(&slots_[i].spatializer);
            std::construct_at(&slots_[i].spatializer);
            slots_[i].active = true;
            return i;
        }
    }
    return -1;
}

void AudioScene::update_source(int id, Eigen::Vector3f world_position, float gain)
{
    if (id < 0 || id >= k_max_sources || !slots_[id].active) return;
    slots_[id].source.world_position = world_position;
    slots_[id].source.gain           = gain;
}

void AudioScene::remove_source(int id)
{
    if (id < 0 || id >= k_max_sources) return;
    slots_[id].active = false;
}

void AudioScene::set_listener(Eigen::Vector3f position, Eigen::Quaternionf orientation)
{
    listener_pos_ = position;
    listener_ori_ = orientation;
}

void AudioScene::mix(float* stereo_out, int frames)
{
    std::memset(stereo_out, 0, sizeof(float) * static_cast<size_t>(frames) * 2);

    for (int i = 0; i < k_max_sources; ++i) {
        if (!slots_[i].active || !slots_[i].source.synth) continue;

        Slot& slot = slots_[i];
        Eigen::Vector3f rel = listener_ori_.inverse() * (slot.source.world_position - listener_pos_);
        float dist = rel.norm();
        slot.spatializer.set_position(dist > 1e-6f ? rel / dist : Eigen::Vector3f{0, 0, -1}, dist);

        float mono[256];
        slot.source.synth->fill(mono, frames);

        float g = slot.source.gain;
        for (int n = 0; n < frames; ++n)
            mono[n] *= g;

        slot.spatializer.process(mono, stereo_out, frames);
    }
}
