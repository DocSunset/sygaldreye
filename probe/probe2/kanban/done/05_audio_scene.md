# audio_scene component

A list of positioned audio sources that mixes into the stereo output buffer each callback. Owns the `HrtfSpatializer` per source and coordinates updates between the scene thread (72 Hz, updates positions) and the audio callback thread (~200 Hz, reads positions and synthesises samples).

## Approach

- `audio_scene.hpp` declares:
  ```cpp
  struct AudioSource {
      Eigen::Vector3f world_position;
      float           gain = 1.0f;
      // Pointer to a synth that produces mono samples.
      // Owned externally; must outlive the AudioScene.
      class MonoSynth* synth = nullptr;
  };

  class AudioScene {
  public:
      static constexpr int k_max_sources = 16;

      // Scene thread: register or update a source. Returns a source ID.
      int  add_source(AudioSource const&);
      void update_source(int id, Eigen::Vector3f world_position, float gain = 1.0f);
      void remove_source(int id);

      // Scene thread: update listener pose (head position + orientation).
      void set_listener(Eigen::Vector3f position, Eigen::Quaternionf orientation);

      // Audio callback thread: mix all live sources into stereo_out.
      // stereo_out is cleared before mixing. frames is the block size.
      void mix(float* stereo_out, int frames);
  };
  ```
- `MonoSynth` is a pure-virtual base (declared in `audio_scene.hpp`) with a single method `void fill(float* out, int frames)`. Each sound generator inherits it.
- Scene-to-audio communication: same lock-free triple-buffer pattern as `hrtf_spatializer` for each source's position/gain.
- Fixed-capacity (`k_max_sources`) to avoid audio-thread allocation.
- `audio_scene.test.cpp` (host): add a 440 Hz sine source at a known position, call `mix`, verify output is non-zero and stereo asymmetric.

## Acceptance

- Multiple simultaneous sources mix correctly without clipping at moderate gain values
- Removing a source mid-stream produces silence for that source within one callback block
- No allocation in `mix()`
- `audio_scene.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `hrtf_spatializer` (item 04)
- `synth_core` (item 01)
