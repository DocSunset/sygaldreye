# Name the hand cube scale magic number in app.cpp

**File:** `components/app/app.cpp`
**Principle:** Magic numbers embedded in expressions must be replaced with named `constexpr` constants so that the intent is self-documenting and adjustments require changing only one definition.

`android_main` already uses named `HAND_CUBE_OFFSET_*_CM` constants for the positional offset, but the scale `0.035f` (line ~72: `scale_m(0,0) = scale_m(1,1) = scale_m(2,2) = 0.035f`) is unnamed. It should be `constexpr float HAND_CUBE_SCALE = 0.035f;` placed alongside the other hand-cube tuning constants so all adjustable parameters are visible in one place.

## Acceptance
- `HAND_CUBE_SCALE` is declared as a `constexpr float` adjacent to the other `HAND_CUBE_OFFSET_*` constants
- The matrix diagonal assignments use `HAND_CUBE_SCALE` instead of `0.035f`
- No behaviour change; build succeeds with no new warnings
