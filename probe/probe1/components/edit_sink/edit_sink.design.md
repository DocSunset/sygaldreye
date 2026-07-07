# edit_sink

The editor's edit drain (`kanban/ready/vr_editor_decomposition.md` S2):
consumes structured edit-op commands and pushes them onto the SAME edit queue
`PeerCore::collect_edits` drains. Replaces `EditorNode`'s whole-graph-JSON
splicing — gesture nodes emit small structured ops here instead. The op
string surgery itself lives in `signal_graph_edit` (`apply_edit_op`), applied
in `collect_edits` against the live graph.

## Ports

- Inputs: `command` — `normalled_in<std::string>`, one op-command JSON.
  Deduped on rising edge (a held command pushes once).
- Outputs: —
- Sources: gesture nodes (`palette`, `wire_drag`, `slider_drag`,
  `dwell_delete`, `undo_node`) emit op-command text.
- Destinations: the edit queue, injected by the shell
  (`PeerCore::pump_contexts`).
- Temporal couplings: `set_context` before `tick`; the op is drained at the
  shell's cadence and applied at the next `begin_frame` swap.
- Intended seams: the op vocabulary (in the header) is the contract; new
  gestures add ops without touching this node.

## Op vocabulary

- `{"op":"add_node","type":"<t>"[,"id":"<id>"]}`
- `{"op":"remove_node","id":"<id>"}`
- `{"op":"add_edge","from":"<n.p>","to":"<n.p>"}`
- `{"op":"remove_edge","from":"<n.p>","to":"<n.p>"}`
- `{"op":"set_param","id":"<id>","port":"<p>","value":<f>}`
- a whole graph object passes through verbatim (old editor full edits).

## Requirements

- Resource-holder (`lift_kind == resource_holder`): owns the queue seam.

## Allowed dependencies

`sygaldry_endpoints`, `event_queue`.

## Owning package

`scene`
