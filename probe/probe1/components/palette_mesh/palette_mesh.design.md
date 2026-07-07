# palette_mesh

The node palette as geometry (vr_editor_decomposition.md S5): the backdrop
panel + the current page's type-row labels (header + 15 rows). The render dual
of the palette gesture node — same panel position/paging, so a poke lands on a
row drawn here.

## Ports
- Inputs: `page` (current palette page, from a palette node).
- Outputs: `panel` (Mesh) + `panel_surface` (unlit vertex-color),
  `labels` (Mesh) + `labels_surface` (glyph atlas) — each wired to a draw.
- Sources: the registry type list via `GestureContext`.

## Requirements
- Resource-holder (unliftable): reads the registry type list.
- Panel position/size + paging identical to the palette node + the monolith.

## Allowed dependencies
`editor_layout`, `palette`, `render_payloads`, `tri_mesh`, `text_mesh`,
`sygaldry_endpoints`.

## Owning package
`scene`
