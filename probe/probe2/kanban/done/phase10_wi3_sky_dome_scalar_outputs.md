# WI-3: SkyDome scalar outputs

## Summary
`SkyDome` needs scalar output ports so downstream nodes (e.g. water) can wire to its
sun direction/elevation values.

## Work
In `sky_dome.hpp`, after adding `render` output (WI-2), also add:
```cpp
struct outputs {
    port<"render",           DrawFn>          render;
    port<"sun_elevation_out",float>           sun_elevation_out;
    port<"sun_azimuth_out",  float>           sun_azimuth_out;
    port<"sun_dir",          Eigen::Vector3f> sun_dir;
    port<"sun_color",        Eigen::Vector4f> sun_color;
} outputs;
```

In `sky_dome.cpp` `operator()`, set these from `inputs`:
```cpp
outputs.sun_elevation_out.value = inputs.sun_elevation.value;
// compute sun_dir from spherical coords
float el = inputs.sun_elevation.value;
float az = inputs.sun_azimuth.value;   // if inputs has azimuth; else use 0
outputs.sun_dir.value = Eigen::Vector3f{
    std::cos(el) * std::sin(az), std::sin(el), std::cos(el) * std::cos(az)
};
outputs.sun_color.value = Eigen::Vector4f{1.f, 0.95f, 0.8f, 1.f};
```

NOTE: `SkyDome::inputs` currently has `sun_elevation` and `star_count` and `radius`.
There is no `sun_azimuth` input — use `0.0f` as default or add it.

## Acceptance
- `SkyDome::outputs` has all 5 fields
- `operator()` populates them
- `sh/build.sh` passes
- Commit
