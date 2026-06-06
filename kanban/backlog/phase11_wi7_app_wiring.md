# WI-7: App wiring

Update `app.cpp` to pass `grip_right`, `delta_time_s`, and `thumbstick_left` to `VrEditor::update()`.

## Deliverable

Updated `app.cpp` call site; grip and thumbstick read from OpenXR action states (may require extending `Input`).

## Acceptance

- All new parameters passed correctly
- `GraphEdit` from WI-5/WI-6 routes through existing graph-swap path
- `sh/build.sh` passes
