# hello_cosine — the hand-frozen movement

Owning package: `movements`. Allowed dependencies: `escapement`,
`nodes/{synth_core,ugens}`.
The rung-2 gate artifact: the ch. 1 running example, frozen BY HAND —
osc0 (220 Hz cosine) → vca0.in, lfo0 (0.5 Hz, unipolar, depth 1) →
vca0.gain, vca0 → out — exactly the fixture's topology and defaults, as an
escapement `movement` over plan-owned block buffers. The dac is the render
boundary: the movement's output IS dac0/in. Blessed against
`conformance/fixtures/golden-audio.md`; the machine freezer (rung 8) must
reproduce this movement from the graph, closing the loop.

- Inputs: none at runtime (a sealed movement does one thing).
- Outputs: mono float samples at 48 kHz, block size 128.
- Intended seams: `render(sink)` — the trampoline, `syg render-movement`,
  and `syg tick-audit` all drive the same frozen plan.
