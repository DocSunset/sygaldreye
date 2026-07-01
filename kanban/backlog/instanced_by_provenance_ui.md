# instanced_by metadata + instantiator filter UI

Travis (2026-06-10): nodes should carry an "instanced_by" identifier
(human, claude, companion, ...) so the editor can hide/show nodes by
instantiator — default-hide agent-created plumbing behind a toggle so
humans don't trip over it (or prank it deliberately).

Needs: parse/serialize round-trip of per-node metadata (alongside the
existing provenance block), spawner/editor/claude-tmux setting their own
ids, migration preserving it, and an editor toggle row of instantiators.
Pairs well with movable-pane persistence (same per-node editor metadata).
