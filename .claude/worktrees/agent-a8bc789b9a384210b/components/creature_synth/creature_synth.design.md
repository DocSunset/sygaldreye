# creature_synth

Procedural biological-creature sound synthesizer. Three modes dispatched on `CreatureKind`.

## Ports

**Inputs**
- `set_params(CreatureParams const&)`: runtime parameter update

**Outputs**
- `fill(float* out, int frames)`: mono audio, [-1, 1] normalized

**Temporal couplings**
- `fill()` must be called repeatedly in sequence; phasors and ADSR carry state across calls.

## Requirements

- Cricket: FM burst train gated by slow group envelope; audible silent gaps between groups.
- Bird: carrier frequency interpolates through an 8-step pitch contour over `phrase_duration_s`; second harmonic partial adds fullness.
- Insect: FM buzz amplitude-modulated by a wing-beat sine.
- No allocation in `fill()`.

## Allowed dependencies

- `synth_core`: `Phasor`, `Adsr`, `sine`
- `biquad_filter`: available for future timbral shaping
- `audio_scene`: `MonoSynth` base class

## Owning package

`scene`
