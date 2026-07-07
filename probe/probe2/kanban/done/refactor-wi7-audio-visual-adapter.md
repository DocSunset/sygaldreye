# WI-7: Audio-to-Visual Adapter Node

## Goal

Add a `float_mapper` graph node that maps one `float` range to another, enabling
audio envelope / LFO outputs to drive visual node sliders:

```
[WaterSynth rms_out] ──► [FloatMapper] ──► [WaterSurface foam_threshold]
[AtmosSynth lfo_out] ──► [FloatMapper] ──► [SkyDome sun_elevation]
[MicCapture  rms]    ──► [FloatMapper] ──► [Aurora ripple_amplitude]
```

The node has no internal state; it maps `in_value` from `[in_min, in_max]` to
`[out_min, out_max]` with optional clamp and optional curve exponent.

## Component

`components/float_mapper/`

```cpp
struct FloatMapper {
    struct inputs {
        port<"in", float>    in;
        slider<"in_min",  "", float, -1e6f, 1e6f, 0.f>    in_min;
        slider<"in_max",  "", float, -1e6f, 1e6f, 1.f>    in_max;
        slider<"out_min", "", float, -1e6f, 1e6f, 0.f>    out_min;
        slider<"out_max", "", float, -1e6f, 1e6f, 1.f>    out_max;
        slider<"curve",   "", float, 0.1f,  10.f, 1.f>    curve;
    };
    struct outputs {
        port<"out", float> out;
    };
    void operator()(double);
};
```

## Acceptance Criteria

- Component exists with CMakeLists, hpp, cpp
- Unit test verifies: linear map, clamping, curve exponent
- `float_mapper` is registered in `component_registry` (or equivalent discovery)
- `sh/build.sh` passes

## Notes

This is the lowest-risk item: pure computation, no GL, no XR. Good candidate for a
first iteration since it demonstrates the cross-domain routing pattern with no
risk of breaking existing rendering.
