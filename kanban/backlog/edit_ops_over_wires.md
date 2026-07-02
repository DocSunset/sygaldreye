# Edit ops never flow over wires

`edit_sink` exists (registered, context-fed, tested) but no graph wires
it — zero references across all JSON + embedded graphs. Gesture nodes
push ops straight into `edit_events_` via the injected context, so edit
ops can't be observed / filtered / rewired in-graph. Either route
gesture output over text edges into `edit_sink`, or retire it.
reports/audit_conformability_editor_arc.md §1.
