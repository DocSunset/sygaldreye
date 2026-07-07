# vr_editor: node cards overlap scene + each other

Cards spawn in a fixed row at z=-0.5 spaced 0.45m; with >3 nodes they overlap
scene geometry (water plane cuts through panels at y≈0) and run off to +x.
Palette is at ground level (y=0) — awkward in VR (was it ever visible on
device?). Needs: place editor relative to head pose, raise above ground,
wrap cards in rows or arc.
