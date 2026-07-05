# biquad_filter

Second-order IIR filter with Audio EQ Cookbook coefficient generation.

## Ports

**Inputs:** `tick(float)` — one audio sample per call; `set_coeffs(BiquadCoeffs)` — update filter type/parameters and reset state.

**Outputs:** `tick` return value — filtered sample.

No sources, destinations, or temporal couplings. No intended seams.

## Requirements

- Direct Form II Transposed for numerical stability.
- Factory functions for: low-pass, high-pass, band-pass, notch, low-shelf, high-shelf.
- No dynamic allocation.
- `set_coeffs` resets internal state to avoid transients from stale history.

## Allowed dependencies

None.

## Owning package

`scene`
