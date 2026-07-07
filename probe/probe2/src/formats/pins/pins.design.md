# pins — the frozen format constants

Owning package: `formats`. Allowed dependencies: none (header-only).
Spec: `architecture/14-formats-protocols.md` (FMT-5) — every constant here
was FROZEN 2026-07-05; changing one is a succession of the spec (ADR-025),
never an edit. Every component that needs a format constant reads it here
(define once); `syg pins` projects them for the conformance test.
