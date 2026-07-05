# noise

Procedural noise functions for CPU use.

## Ports

### Inputs
- `perlin(x, y)` — 2D position
- `perlin(x, y, z)` — 3D position
- `fbm(x, y, octaves, lacunarity, gain)` — 2D position and fBm parameters

### Outputs
- Float noise value, approximately in [-1, 1] for `perlin`; scaled sum for `fbm`

### Sources
None.

### Destinations
None.

### Temporal couplings
None. All functions are pure.

### Intended seams
None.

## Requirements

- `perlin` output range approximately [-1, 1]
- Deterministic: same inputs produce same output
- C1-continuous (smooth): gradient noise with fade curve
- `fbm` sums octaves with geometric amplitude decay
- No external dependencies, no GL, no XR, no Android
- Host-buildable

## Allowed dependencies

None.

## Owning package

`scene`
