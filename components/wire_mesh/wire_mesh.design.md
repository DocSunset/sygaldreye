# wire_mesh

Draws the patch wires of the VR editor: cubic beziers tessellated to
GL_LINES with per-vertex color. First peel of the VrEditor monolith
(`kanban/ready/vr_editor_decomposition.md` slice 1): wire *data* comes
from the editor; wire *appearance* lives here, swappable live.

## Ports

- Inputs: —
- Outputs: —
- Sources: `wires` — `in<Span>`, N×10 rows of `{from.xyz, to.xyz, rgba}`,
  produced by `editor_wires` (or anything else shaped like it).
- Destinations: `surface` + `mesh` — `out<Surface>`/`out<Mesh>`, consumed
  by a `draw` node and rendered by `render_region` (render-as-nodes).
- Temporal couplings: geometry is tessellated at tick; render_region draws
  the mesh after the tick on the GL thread.
- Intended seams: row layout is the contract; alternative wire renderers
  (tubes, glow) replace this node without touching the editor.

## Requirements

- One held TriMeshData refilled in place from the wires span each frame and
  touch()ed, so render_region re-uploads it once per frame (never per eye);
  GL lives in render_region (no GL in this node).

## Allowed dependencies

`sygaldry_endpoints`, `gl_program`.

## Owning package

`scene`
