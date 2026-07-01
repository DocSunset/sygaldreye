# card_widgets_mesh

The card internals as geometry (vr_editor_decomposition.md S5): every card's
per-port handles (coloured by kind, inputs left / outputs right) and scalar
sliders (track + thumb) as ONE vertex-coloured MeshPtr. The rendering dual of
the gesture nodes — both read editor_layout off the graph, so a drawn handle is
exactly where wire_drag hit-tests.

## Ports
- Outputs: `mesh` (MeshPtr geometry) — wire into a vertex_color_mesh for the
  Surface, then a draw node.
- Sources: the live graph + shared overrides, via `GestureContext`.

## Requirements
- Resource-holder (unliftable): observes the graph it lives in.
- Geometry/colours match the monolith (0.006 m handles, kind colours, slider
  track + thumb at the value).

## Allowed dependencies
`editor_layout`, `render_payloads`, `tri_mesh`, `sygaldry_endpoints`.

## Owning package
`scene`
