# fanin: 4 bounded slots, not a span gather

The engine's published fan-in is in0..in3 string concat; the block path
already has span-style gather. Unify when the structured lane grows
span/cell_rank semantics. Rung-7 audit, 2026-07-05.
