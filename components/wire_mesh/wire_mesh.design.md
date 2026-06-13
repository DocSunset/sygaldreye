# wire_mesh

Draws the patch wires of the VR editor: cubic beziers tessellated to
GL_LINES with per-vertex color. First peel of the VrEditor monolith
(`kanban/ready/vr_editor_decomposition.md` slice 1): wire *data* comes
from the editor; wire *appearance* lives here, swappable live.

## Ports

- Inputs: —
- Outputs: —
- Sources: `wires` — `in<Span>`, N×10 rows of `{from.xyz, to.xyz, rgba}`,
  produced by `editor.wires` (or anything else shaped like it).
- Destinations: `render` — `out<DrawFn>`, collected by the shell's draw
  pass like every visual node.
- Temporal couplings: the DrawFn runs after the tick on the GL thread;
  vertices are copied/tessellated at tick so the producer's span may be
  rewritten freely.
- Intended seams: row layout is the contract; alternative wire renderers
  (tubes, glow) replace this node without touching the editor.

## Requirements

- Zero allocation in the draw path (vertex buffer reused, GL_STREAM_DRAW).
- Lazy GL init inside the DrawFn (GL context exists only on the render
  thread).

## Allowed dependencies

`sygaldry_endpoints`, `gl_program`.

## Owning package

`scene`
