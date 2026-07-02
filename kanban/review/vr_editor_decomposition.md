# VrEditor decomposition into the graph (discussed 2026-06-12)

The monolith (~960 lines, 6 files) owns: palette, node cards, port
handles, sliders, wires, gestures (wire-drag, card-move, dwell-delete,
undo). Every UX hotfix tonight needed a reinstall because of it
(live_update_gap.md blocker 1).

## The deep thing: the editor is meta

A node sees its ports; the editor needs the WHOLE graph as data and
emits edits against it. Today: set_context() injecting raw pointers.
Decomposed: `graph_source` (publishes the live graph model — nodes,
edges, types, schemas — as data) + `edit_sink` (consumes edit
commands). The /meta-graph route gestures at the right shape: the
editor can BE a separate graph observing/editing the main one, which
also avoids self-reference (cards for the editor's own nodes).

## Enablers

1. span/array payloads + text payloads (endpoints v6 / overnight 2)
2. conformability.md — cards = lifted card-subgraph over
   graph_source.nodes, keyed by node id; handles per port; palette
   rows per type. No replicator node needed.
3. device plugin dlopen crash fix — the difference between editing the
   editor live and tonight's reinstall ritual.

Already exist as nodes: ui_slider/ui_button/ui_pane/text_label,
poke_stick/poke_button (collision interaction proven in-graph),
grab_detector (drag gestures).

## Peel order (each slice = plugin + graph edit)

1. **Wires** — DONE 2026-06-12 (a099a98): editor.wires span →
   wire_mesh node. Device shell's embedded VrEditor deleted; editor +
   controller sources + wires are graph content on both shells.
   In-headset interaction check pending.
2. Hover labels.
3. Sliders bound via edit messages (structured edit ops replace
   whole-graph JSON splicing).
4. Cards + palette via lifting.
5. Gesture state machines.
6. Delete VrEditor when empty.

## Status (2026-07-01 — in review)

Delivered in 87496a2 (monolith deleted; editor.json boots both shells);
in-headset verification still pending with Travis. The 2026-07-01
remediation arc fixes the audit's editor defects on this branch:
slider-drag rerouted through queue_param, ABI v9 generic context seam
replacing pump_contexts' per-type dispatch, shell registration parity,
seq draw-order wiring. Undone plan slices filed as backlog
(card_subgraph_decomposition, edit_ops_over_wires,
card_positions_as_graph_content). See
reports/audit_conformability_editor_arc.md §1.
