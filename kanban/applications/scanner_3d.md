# 3D scanner + model viewer for LLMs (brainstorm 2026-06-21)

Application note, not yet a work item.

Use the Quest's cameras/depth to scan real objects and spaces into 3D
models from inside the graph — capture nodes feeding reconstruction
(point cloud / mesh / Gaussian splat) as wirable graph content.

Paired use: a model viewer for LLMs — let an LLM "see" and reason about
a 3D model (the scanned result, or any imported asset) by rendering
controllable views into the graph and back out to the model. Closes a
loop with the speech/text nodes: talk to the LLM about the thing you
just scanned, in the room with it.

Open questions: depth/camera access on Quest (passthrough API limits),
reconstruction method + where it runs (on-device vs host worker), and
how the LLM consumes views (rendered stills as image edges? a
camera-pose port the model drives?).
