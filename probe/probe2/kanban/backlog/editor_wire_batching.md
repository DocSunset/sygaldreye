# Editor wire drawing tanks framerate (root cause of the 22 FPS playground)

> RESOLVED 2026-06-13 (a099a98). Wires are now graph content: the editor
> emits an N×10 span, the `wire_mesh` node tessellates and draws every
> wire through ONE persistent streamed VBO + a single GL_LINES draw per
> eye. The per-wire VAO/VBO create/delete churn is gone by construction.
> The interim PrimBatch persistent-VBO fix (below) also shipped and is
> retained for quads. Re-listen for distortion under a full payload on
> device; the underrun theory remains unconfirmed. Move to done after
> the in-headset FPS check.

Found by device bisection 2026-06-12. The render thread pegs at ~96% CPU
and FPS tracks EDGE COUNT, not node count or scene content:

- 29 cards, 0 wires: 72 FPS
- 25 trivial nodes, 24 wires: 39 FPS
- 32-node / 32-edge playground: 22 FPS

`draw_wire` (vr_editor_draw.cpp) creates a VAO+VBO, uploads 15 bezier
points, draws, and DELETES both — per wire, per eye, per frame. Adreno
driver churn on create/delete is the cost. `draw_quad` churn measured
fine at ~90 quads/eye (don't fix what isn't broken).

Fix: accumulate all wire vertices per frame into one persistent
VBO (orphaned glBufferData), color as a vertex attribute, single draw.
~40 lines in vr_editor_draw.cpp. C++ → next rebuild batch.

Suspected to ALSO be the audio distortion: with the render thread
saturated, the AudioTrack callback underruns (Travis heard "horribly
distorted" chimes only in the full playground; the same synth chain in
a small graph probes clean). Verify after the fix by ear.

Update, same night: Travis reports poke chimes sound FINE now (full
playground + probes, still wire-throttled to ~22 FPS) — so the
underrun-distortion theory is unconfirmed; distortion currently not
reproducible. Don't claim the wire fix cures it; just re-listen after.
