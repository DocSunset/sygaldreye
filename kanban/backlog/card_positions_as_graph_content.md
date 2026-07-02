# Card drag positions are shell-owned state

Card positions live in `PeerCore::editor_overrides_`, written directly
by wire_drag.cpp:66 — not serialized, not undoable, invisible to remote
peers/agents, lost on restart. graph_source.design.md claims positions
are "updated by edit ops"; they are not. Make positions graph content
(set_param / a position op through the edit queue).
reports/audit_conformability_editor_arc.md §1.
