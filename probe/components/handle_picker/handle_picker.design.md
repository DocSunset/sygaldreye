# handle_picker

The hover-label picker (vr_editor_decomposition.md S3, from vr_editor.cpp hover
loop). The nearest port handle / slider to the tip → a legible label + its world
position, for a text_label to render. Feedback, not an edit.

## Ports
- Sources: `pos` (vec3), `rot` (quat) — right controller; the graph via context.
- Outputs: `label` (text, "" when nothing near), `pos_out` (vec3 anchor).
- Temporal couplings: `set_context` each frame.

## Requirements
- Resource-holder (unliftable): observes the graph it lives in.
- Nearest within 0.05 m; an actively-near slider shows a value readout.

## Allowed dependencies
`editor_layout`, `sygaldry_endpoints`, `signal_graph`.

## Owning package
`scene`
