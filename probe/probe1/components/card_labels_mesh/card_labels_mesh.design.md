# card_labels_mesh

Every card's id label as ONE glyph Mesh + the MSDF atlas Surface
(vr_editor_decomposition.md S5). Reuses the glyph layout/atlas machinery
text_label uses, batched across cards (text_label renders ONE string). Labels
sit at the monolith's per-card anchor.

## Ports
- Inputs: `scale` (text size; 0.5 ≈ the monolith's 0.018 m × 0.5).
- Outputs: `mesh` (Mesh), `surface` (Surface, atlas-sampling) — wire to a draw.
- Sources: the live graph + overrides, via `GestureContext`.

## Requirements
- Resource-holder (unliftable): observes the graph it lives in.
- Glyph layout + atlas identical to text_label.

## Allowed dependencies
`editor_layout`, `render_payloads`, `tri_mesh`, `text_mesh` (glyph_layout),
`sygaldry_endpoints`.

## Owning package
`scene`
