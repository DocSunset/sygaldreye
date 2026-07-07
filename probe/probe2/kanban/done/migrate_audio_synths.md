# migrate_audio_synths

Port seven audio synth components to the node graph ABI so they can be instantiated and parameterized at runtime via `ComponentRegistry`.

## Components

- `components/atmos_synth/atmos_synth.hpp` / `.cpp`
- `components/rain_synth/rain_synth.hpp` / `.cpp`
- `components/fire_synth/fire_synth.hpp` / `.cpp`
- `components/engine_synth/engine_synth.hpp` / `.cpp`
- `components/water_synth/water_synth.hpp` / `.cpp`
- `components/creature_synth/creature_synth.hpp` / `.cpp`
- `components/chime_synth/chime_synth.hpp` / `.cpp`

## Pattern

These inherit from `MonoSynth` and implement `fill(float*, int)`. They produce audio, not visuals. They do NOT go into the render path.

For each synth:

1. Add to `.hpp`:
   ```cpp
   static consteval std::string_view name()          { return "snake_case_name"; }
   static consteval std::string_view source_header() { return "components/X/X.hpp"; }
   static consteval std::string_view source_cpp()    { return "components/X/X.cpp"; }

   struct inputs {
       slider<"label", "", float, fp(min), fp(max), fp(init)> field;
       // float params only; skip int arrays (e.g. pitch_contour), skip sample_rate
   } inputs;
   ```

2. Add `void operator()(double /*time*/)` that syncs `inputs.*.value` into a params struct and calls `set_params(p)`.

3. Keep existing constructor and `set_params()` API untouched.

## Params to expose per synth

### AtmosSynth (`AtmosParams`)
- `wind_speed` [0.0, 10.0, init 1.0]
- `base_freq` [50.0, 2000.0, init 300.0]
- `brightness` [0.0, 1.0, init 0.5]
- Skip `sample_rate`

### RainSynth (`RainParams`)
- `rate` [0.0, 1000.0, init 200.0]
- `drop_freq` [500.0, 10000.0, init 4000.0]
- `decay_s` [0.001, 0.5, init 0.01]
- Skip `sample_rate`

### FireSynth (`FireParams`)
- `intensity` [0.0, 5.0, init 1.0]
- `crackle_rate` [0.0, 200.0, init 40.0]
- Skip `sample_rate`

### EngineSynth (`EngineParams`)
- `rpm` [100.0, 8000.0, init 800.0]
- `roughness` [0.0, 1.0, init 0.3]
- `load` [0.0, 1.0, init 0.5]
- Skip `cylinders` (int), skip `sample_rate`

### WaterSynth (`WaterParams`)
NOTE: `WaterParams` here is the *audio* one (`components/water_synth/`), NOT the visual one from `water_surface.hpp`. Confirm the include path is correct.
- `flow_speed` [0.0, 10.0, init 1.0]
- `wave_rate` [0.0, 2.0, init 0.2]
- `wave_height` [0.0, 2.0, init 0.5]
- `brightness` [0.0, 1.0, init 0.5]
- Skip `sample_rate`

### CreatureSynth (`CreatureParams`)
- `carrier_freq` [100.0, 10000.0, init 4500.0]
- `fm_index` [0.0, 20.0, init 2.0]
- `chirp_rate` [0.5, 100.0, init 20.0]
- `phrase_duration_s` [0.1, 10.0, init 2.0]
- Skip `kind` (enum), `pitch_contour` (array), `sample_rate`

### ChimeSynth (`ChimeParams`)
- `strike_rate` [0.0, 10.0, init 0.0]
- `strike_gain` [0.0, 2.0, init 1.0]
- Skip `modes` (array of structs), `mode_count` (int), `sample_rate`

## Registration

After all synths are ported, add to `components/app/app.cpp`:
```cpp
#include "atmos_synth.hpp"
// etc.
state.registry_.register_builtin(make_descriptor<AtmosSynth>());
// etc.
```

And add each lib to `components/app/CMakeLists.txt` target_link_libraries.

## Notes

- Audio synths are not wired into the audio pull chain by this work item — that's a later task. This item only makes them graph-instantiable.
- `WaterParams` name collision: `water_surface.hpp` also defines `WaterParams`. The audio `WaterParams` is in the `water_synth` component. Be careful with includes — `water_synth.hpp` must be included in a TU that doesn't also include `water_surface.hpp`, or use anonymous namespace / rename. Check if there's already a collision before deciding. If there is, rename the audio one `WaterSynthParams` in `water_synth.hpp` and update its `.cpp`.
- Warnings as errors — fix any that arise
- Build must pass for Android (aarch64) after all changes
- `inputs` must be aggregate: no constructors, no private members
