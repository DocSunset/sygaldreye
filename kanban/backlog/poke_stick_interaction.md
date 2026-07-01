# Collision-based interaction: the poke stick

Travis's preferred model (replaces pointing for near-field): a small
narrow stick attached to the controller; when the stick INTERSECTS an
interactable and the user then presses/pulls, the action fires. Pointing
ray stays for far-field (palette across the room) but proximity wins
up close.

Design notes:
- The stick is a NODE: controller pose in → stick geometry render out +
  a collision volume; interactables (ui_slider/knob/button/handles)
  test against it.
- This wants the collision test graph-expressed: stick.tip_pos (vec3)
  edges into widgets' existing ray_pos-style inputs — widgets already
  take pose+trigger via edges, so a poke variant is mostly NEW WIDGET
  NODES, not editor surgery.
- Ship it as the FIRST LIVE PLUGIN (see live_update_gap.md): compile
  poke_stick + knob nodes as .so via companion/compile_node.py, POST to
  /plugins on the running headset, wire by graph edit. Zero reinstall.
