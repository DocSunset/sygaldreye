# migrate_visual_components

Port six visual scene components to the node graph ABI so they can be instantiated, parameterized, and composed at runtime via `ComponentRegistry`.

## Components

- `components/lissajous/lissajous.hpp` / `.cpp`
- `components/aurora/aurora.hpp` / `.cpp`
- `components/chladni/chladni.hpp` / `.cpp`
- `components/terrain_generator/terrain_generator.hpp` / `.cpp`
- `components/particle_system/particle_system.hpp` / `.cpp`
- `components/reaction_diffusion/reaction_diffusion.hpp` / `.cpp`

## Pattern (established by WaterSurface / SkyDome)

For each component:

1. Add to `.hpp`:
   ```cpp
   static consteval std::string_view name()          { return "snake_case_name"; }
   static consteval std::string_view source_header() { return "components/X/X.hpp"; }
   static consteval std::string_view source_cpp()    { return "components/X/X.cpp"; }

   struct inputs {
       slider<"label", "", float, fp(min), fp(max), fp(init)> field;
       // ... float knobs only; skip Eigen::Vector*, int arrays, color fields
       // int params (samples, mode_m, mode_n, etc.) may use float slider cast to int in operator()
   } inputs;
   ```

2. Add `void operator()(double time)` in `.cpp` that syncs `inputs.*.value` into `params_` then calls existing `update(time)`.

3. Keep existing `static create(params)` factory untouched (backwards compat). Default constructor calls `create({})` internally or leaves the object uninitialized — match whatever each class already does.

## Params to expose per component

### Lissajous (`LissajousParams`)
- `freq_x` [0.5, 20.0, init 3.0]
- `freq_y` [0.5, 20.0, init 4.0]
- `freq_z` [0.0, 5.0, init 0.5]
- `phase_x` [0.0, 6.283, init 0.0]
- `amp` [0.1, 5.0, init 1.0]
- `samples` as float slider [100, 10000, init 4000] — cast to int in operator()

### Aurora (`AuroraParams`)
- `curtain_width` [50.0, 1000.0, init 400.0]
- `altitude_base` [0.0, 200.0, init 60.0]
- `altitude_height` [10.0, 500.0, init 170.0]
- `ripple_amplitude` [0.0, 100.0, init 22.0]
- `ripple_speed` [0.0, 5.0, init 0.5]
- Skip `curtain_count`, `curtain_segments_w/h` (int, structural — rebuilding geometry would be expensive; leave as defaults)

### Chladni (`ChladniParams`)
- `omega` [0.0, 20.0, init 1.0]
- Skip `grid_n`, `mode_m`, `mode_n` (int, structural), skip color vectors

### TerrainGenerator / TerrainRenderer (`TerrainParams`)
- `height_scale` [0.0, 100.0, init 20.0]
- `lacunarity` [1.0, 4.0, init 2.0]
- `gain` [0.1, 1.0, init 0.5]
- `noise_offset_x` [-100.0, 100.0, init 0.0]
- `noise_offset_z` [-100.0, 100.0, init 0.0]
- Skip `grid_w/h`, `octaves` (int), skip color bands, Light, Material
- Note: `TerrainRenderer` has no `update()` method; params are baked at creation. The `operator()` should call `set_sun(params_.sun)` and update only the float params that don't require mesh rebuild. For the knobs that do require rebuild (height_scale, noise_offset_*), operator() can mark dirty and recreate via `TerrainRenderer::create(params_)` — OR simply skip for now and only expose `sun.intensity` as a live knob. Read the .cpp to decide.

### ParticleSystem (`EmitterParams`)
- `ParticleSystem` uses `EmitterParams` not a single `Params` struct. Expose a wrapper node:
  - `emit_rate` [0.0, 500.0, init 50.0]
  - `lifetime_min` [0.1, 10.0, init 0.5]
  - `lifetime_max` [0.1, 10.0, init 2.0]
  - `size_start` [0.01, 1.0, init 0.05]
  - `size_end` [0.0, 1.0, init 0.0]
- Default-construct with `capacity=1000`. In operator(), sync inputs → emitter, call `update(dt)` where `dt = time - prev_time_`.
- Keep `set_emitter()` API intact.

### ReactionDiffusion (`RDParams`)
- `Du` [0.0, 1.0, init 0.16]
- `Dv` [0.0, 1.0, init 0.08]
- `F` [0.0, 0.1, init 0.060]
- `k` [0.0, 0.1, init 0.062]
- `dt` [0.1, 2.0, init 1.0]
- `steps_per_frame` as float [1.0, 32.0, init 8.0] — cast to int in operator()
- Skip color fields

## Registration

After each component is ported, add to `components/app/app.cpp`:
```cpp
#include "lissajous.hpp"
// etc.
state.registry_.register_builtin(make_descriptor<Lissajous>());
// etc.
```

And add each lib to `components/app/CMakeLists.txt` target_link_libraries.

## Constraints

- Warnings as errors — fix any that arise
- Each .cpp < 100 lines substantive code
- Build must pass for Android (aarch64) after all changes
- Do NOT restructure internals — minimal invasive changes only
- `inputs` must be aggregate: no constructors, no private members
