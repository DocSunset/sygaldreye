# stage0 — the frozen realization

Owning package: `stage0`. Allowed dependencies: `crown`, `organs` (linked
natives via the generated registration TU).
Spec: ch. 6 — stage 0 is a frozen realization of the boot tape: the tape is
EMBEDDED at build time (a build-system derivation; provenance to the
source file), replayed by the crown at boot; the resulting parked graph
REJECTS runtime edits (SZ-5.2 — the one supervisor that cannot be edited
away); `unfreeze` recovers tape + native manifest by hash (SZ-4). Boot
requires no store: an empty object directory reaches a live engine graph
from the embedded tape alone (SZ-7), through op application only (SZ-8).
