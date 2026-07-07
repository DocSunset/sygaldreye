# Inlets & defaults: retire "param" as a category (ratified 2026-06-12)

Design: edge_executor.design.md "Inlets and defaults". Ladder below is
ordered so each rung ships alone, all targets, tests first.

## 1. Subgraph inlet-params (the purest case, do first)
- SubgraphDescriptor gains serialize/deserialize: deserialize maps
  {"inlet_name": value} → cache_inlet (scalar + text v1; vec later with
  rung 5); serialize dumps the cached defaults back.
- Result: {"type":"chime_synth_g","params":{"freq":880,"rate":2}} works —
  presets are parametric abstractions, no wiring needed for constants.
- Test: spawn chime with freq 880, spectrogram fundamental moves; graph
  round-trip preserves the override; migration keeps it across edits.
- Editor: subgraph cards already get sliders via the derived schema
  (kind+min/max flow from inner ports) — verify, fix if widget seeding
  needs the new serialize.

## 2. Default/live split in sygaldry_endpoints
- slider/toggle/text (and rung-5 vecs) gain a `default` alongside
  `value`. deserialize sets BOTH (an edit is a new default); typed
  setters (edges, latches) set `value` only; serialize emits `default`.
- No ABI change: /param and editor widgets already go through
  deserialize-shaped JSON; edges already go through set_*_in.
- Emergent precedence (document + test, no extra code): /param on a
  connected inlet updates default; the edge rewrites value next tick —
  edge owns live, edit owns fallback. Disconnect → next deserialize or
  a "revert to default" tick? DECIDE: on edge removal, migration
  re-applies params → value=default automatically on the swap. Verify
  with a test (connect lfo→freq, set default 2000, remove edge, expect
  freq 2000 after swap).
- Fixes serialize-mid-modulation bug; add regression test: lfo→slider,
  serialize, assert JSON shows default not the lfo sample.

## 3. Vec/quat persistence (param_registry)
- from_json/to_json learn Eigen vec2/3/4 + quat fields ([..] arrays).
- port<vec3>-style inlets become persistable → spatialize.pos placeable
  by hand; remote bridge "set" path simplifies (arrays already parse).
- Schema: vec inlets keep kind; defaults visible via serialize.

## 4. Editor: affordances derived from payload
- Widget per unconnected persistable inlet: slider (scalar+range),
  toggle, vec3 triple (gizmo later), momentary fire button for bang
  inlets (fires set_scalar_in 1-then-0 or queue event; NOT persisted).
- Connected inlets render as meters (live value from g.values via the
  edge's from_key) instead of editable widgets; revert on disconnect.
- Widget edits route through per-port mini-JSON deserialize (sets
  default+value) instead of set_scalar_in (which is now edge-only).
- This is also editor-recomposition fuel: widget set derives from
  schema alone — no per-type editor code.

## 5. Cleanups & docs
- Remove is_wirable() special-casing where it duplicated the derived
  rule; param_registry doc note ("the defaults serializer").
- VCV-expectation note (override, not sum) already in design doc.
- text inlets: revisit when text edges land (executor open question).
- /params legacy route: confirm nothing uses it; delete.

## Explicit non-goals
- No structural creation args (rejected, see design doc).
- No format migration: existing graph JSON is already the defaults map.
