# Audio-region (block) lifting

Lifting a host in the block region is now a plan-time hard error
(2026-07-01 remediation arc; previously it silently never lifted —
`render_block` ignores LiftGroups while `wire_plan` nulls the input,
so N-voices ran on defaults, audit §2). Actually supporting
block-region lifts — the classic N-voices case — is future work:
LiftGroup replay in the block scheduler + RT-safe (off-callback)
instance allocation. reports/audit_conformability_editor_arc.md §2.
