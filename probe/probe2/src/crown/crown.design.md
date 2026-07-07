# crown — the minimal self-modification

Owning package: `crown` (stratum 1). Allowed dependencies: `escapement`
only (plus std containers for the mutable table). NO vocabulary, NO codec,
NO resolver — the tape needs no parser (ADR-028); natives arrive LINKED
(the registration TU's business, SZ-2).

- The plan as mutable data: an instance table (id → linked native + state +
  out buffers), an edge list, a defaults journal.
- ONE applier primitive over the five ops — instantiate-linked-native,
  link, unlink, remove, write-default — executed at tick boundaries (the
  op inlet is a queue; `tick()` drains it first: atomicity free at control
  rate).
- The tape reader: newline records, space fields, percent-escapes (the
  pinned FMT-3 grammar) → ops into the inlet.

Deliberately without policy, transactions, or cleverness — the full slot
(rung 4) is the crown grown up. A sealed movement omits this entirely.

- Inputs: op records (tape or inlet); a linked-native registry.
- Outputs: the ticked graph's buffers; a serialization of the table
  (`syg replay-tape`'s face).
- Temporal couplings: ops apply only between ticks; instances process in
  instantiation order (the tape's responsibility at this rung).
- Intended seams: the arbiter (ADR-023) is this queue with attribution;
  the slot organ (rung 4) adds transactions + state migration.
