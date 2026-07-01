# Node decomposition story (epic — needs refinement with Travis)

Travis (2026-06-12): beyond the editor monolith, "quite a number of
nodes deserve to be decomposed" into graphs of primitives. Standing
rule (ratified): if a hardcoded node already has an equivalent graph
decomposition, DELETE the node — done for the seven baked synths and
the aurora monolith (aurora.json = five aurora_curtain nodes).

Candidates to refine (decompose into subgraphs of primitives, then
delete the C++):
- lissajous / chladni — parametric curves: math nodes → span → mesh?
  (needs span-consuming line/curve renderer primitive)
- terrain / particle_system / reaction_diffusion — sim + render
  monoliths; rd_gpu + rd_renderer already show the split direction
- hand — pose source + passthroughs; could be xr source + math nodes
- spatialize — mc chain (hrtf per channel) once lifting handles it
- spectrogram — fft primitive + ring texture writer
- ui widgets — panel primitive + hit-test/interaction nodes
- scatter — random source + span ops once span algebra exists

Each decomposition wants its missing PRIMITIVES identified first —
that's the real payoff (the primitive vocabulary grows, the monolith
count shrinks). Revisit with Travis before acting.
