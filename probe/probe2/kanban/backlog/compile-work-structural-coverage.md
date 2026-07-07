# note_compile_work: structural coverage, not opt-in instrumentation

Only lift_expand/infer_regions/expand_subgraphs self-report, and the audit
window opens after transient-plan construction — placement chosen by the
audited code. A rewritten bespoke walk that inlines its own logic passes at
zero. Wanted: structural coverage (e.g. any graph_doc traversal outside a
hook counts) + window owned by the harness. Rung-7 audit, 2026-07-05.
