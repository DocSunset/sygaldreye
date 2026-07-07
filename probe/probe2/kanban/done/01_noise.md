# noise component

Procedural noise functions (Perlin and/or simplex) for use on the CPU. The universal primitive behind terrain height, wind displacement, water surface, fire flicker, firefly drift, cloud density, and every other natural-feeling irregularity.

## Approach

- `noise.hpp` declares:
  - `float perlin(float x, float y)` — 2D gradient noise, range approximately [-1, 1]
  - `float perlin(float x, float y, float z)` — 3D variant
  - `float fbm(float x, float y, int octaves, float lacunarity, float gain)` — fractional Brownian motion (octave sum), the standard "natural" noise
- `noise.cpp` implements using the classic permutation-table approach (Gustavson / Perlin reference). No external dependencies.
- `noise.test.cpp`: verify output range, smoothness (adjacent samples differ by less than a bound proportional to step size), reproducibility (same inputs → same output), and that `fbm` with 1 octave equals `perlin`.

## Acceptance

- Host-buildable and host-testable (no GL, no XR, no Android)
- `fbm(x, y, 4, 2.0, 0.5)` produces visually coherent noise when sampled on a grid (manual inspection)
- All tests pass
- `noise.design.md` present; `.cpp` under 100 lines of substantive code
