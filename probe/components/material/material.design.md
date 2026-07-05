# material

Blinn-Phong surface material parameters for a renderable object.

## Ports

- Inputs: none
- Outputs: none
- Sources: none
- Destinations: consumed by the renderer to shade cube instances
- Temporal couplings: none
- Intended seams: none

## Requirements

- Provide ambient, diffuse, specular coefficients and shininess exponent for Blinn-Phong shading
- Header-only; zero runtime cost

## Allowed dependencies

None (Eigen header-only types only)

## Owning package

`scene`
