# The leaf vocabulary — generated essential natives

**Trigger: the moment LNG-11's lane lands.** Every missing leaf is an
excuse for the next C++ side-door; seed these so "author it as a graph"
is the cheap path from rung 7 onward. Standing work, not a stage —
ADR-033 and ch. 16's ledger promise it (~30 floor one-liners, half the
native count) but no rung schedules it.

- [DONE 2026-07-05] The one-liner stamp lives in the generator
  (`leaves[]` table in generate.cpp → descriptor + native + registration;
  one line per leaf). Seeded 20: mul sub div fmin fmax fmod_ pow_ lt gt
  eq and_ or_ wrap01 abs_ neg not_ clamp01 sqrt_ exp_ tanh_ (block lane;
  `add` predates the stamp; sin/cos ride osc). Value-lane variants and
  the freezer's leaf templates land when the freezer unshelves.
- Schema structs for the catalog's struct-shaped kinds (vec2/3/4, quat,
  mat4, text) per ch. 2's kind-authoring paragraph, committed as kind
  nodes through CMP-9.4's machinery — vocabulary/kinds.json becomes
  generated output for these, hand-authored only for raw-lane kinds.
- Text honest minimum (concat, format, parse) as generated or maturity
  natives, gated on LNG-9/LNG-11.3 payload semantics.

Ledger changes are ordinary commits declaring clauses (ADR-033), never
ADRs. Ch. 16's ledger tracks the set as it grows.
