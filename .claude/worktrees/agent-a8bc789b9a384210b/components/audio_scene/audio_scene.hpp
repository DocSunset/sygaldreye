#pragma once
#include <hrtf_spatializer.hpp>
#include <Eigen/Core>
#include <Eigen/Geometry>

class MonoSynth {
public:
    virtual void fill(float* out, int frames) = 0;
    virtual ~MonoSynth() = default;
};

struct AudioSource {
    Eigen::Vector3f world_position{0, 0, 0};
    float           gain = 1.0f;
    MonoSynth*      synth = nullptr;
};

class AudioScene {
public:
    static constexpr int k_max_sources = 16;

    int  add_source(AudioSource const&);
    void update_source(int id, Eigen::Vector3f world_position, float gain = 1.0f);
    void remove_source(int id);
    void set_listener(Eigen::Vector3f position, Eigen::Quaternionf orientation);
    void mix(float* stereo_out, int frames);

private:
    struct Slot {
        AudioSource      source;
        HrtfSpatializer  spatializer;
        bool             active = false;
    };

    Slot               slots_[k_max_sources];
    Eigen::Vector3f    listener_pos_{0, 0, 0};
    Eigen::Quaternionf listener_ori_{Eigen::Quaternionf::Identity()};
};
