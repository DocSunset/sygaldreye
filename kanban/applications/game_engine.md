# Game engine (brainstorm 2026-06-23)

Application note, not yet a work item.

Use the graph as a game engine — entities, physics, input, rendering,
and game logic as wirable nodes. The patch is the game; authoring
happens in-VR, live. Builds on what's already here (scene, physics
blocks, XR input, render) framed as a general engine rather than a
fixed app.

Open questions: scene/entity model (does the existing block-region
graph suffice, or does it need an ECS layer?), scripting/logic
authoring beyond pure dataflow, and runtime/play vs edit mode
separation.
