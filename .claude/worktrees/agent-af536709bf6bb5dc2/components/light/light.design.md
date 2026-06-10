# light

A plain data structure describing a light source in the scene.

## Owning package

`scene`

## Allowed dependencies

None.

## Ports

### Inputs

None. `Light` is a passive data holder; callers set its fields directly.

### Outputs

None.

### Sources

None.

### Destinations

Consumed by the `renderer` package to shade geometry.

### Temporal couplings

None.

### Intended seams

`LightType` is an enum; new light types (e.g. `Point`, `Spot`) can be added without breaking existing consumers that handle only `Directional`.

## Requirements

- Represent at minimum a directional light with a direction, colour, and intensity.
- Be header-only; no translation unit is required.
- Carry no dependency on OpenXR, OpenGL ES, or Android headers.
