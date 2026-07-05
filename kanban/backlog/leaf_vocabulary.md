# The leaf vocabulary — generated essential natives

**Trigger: the moment LNG-11's lane lands.** Every missing leaf is an
excuse for the next C++ side-door; seed these so "author it as a graph"
is the cheap path from rung 7 onward. Standing work, not a stage —
ADR-033 and ch. 16's ledger promise it (~30 floor one-liners, half the
native count) but no rung schedules it.

- Extend the generator with the one-liner stamp: a kernel-lambda table
  (name, arity, expression) → endpoints struct + native + registration.
  Adding a leaf must cost ONE table line (AUT-3.1's spirit).
- Seed set: arithmetic (add sub mul div mod min max abs pow), trig/exp
  (sin cos tan exp log sqrt), compare/logic (lt gt eq and or not), range
  (clamp lerp scale wrap), const.
- Schema structs for the catalog's struct-shaped kinds (vec2/3/4, quat,
  mat4, text) per ch. 2's kind-authoring paragraph, committed as kind
  nodes through CMP-9.4's machinery — vocabulary/kinds.json becomes
  generated output for these, hand-authored only for raw-lane kinds.
- Text honest minimum (concat, format, parse) as generated or maturity
  natives, gated on LNG-9/LNG-11.3 payload semantics.

Ledger changes are ordinary commits declaring clauses (ADR-033), never
ADRs. Ch. 16's ledger tracks the set as it grows.
