# syg — the conformance harness binary

Owning package: `harness` (conformance adapter, not a node). Allowed
dependencies: the `formats` components; later, crown/organs as rungs demand.
Contract: `conformance/HARNESS.md` — each subcommand reads stdin, writes
stdout, exits nonzero with a message on stderr. Subcommands are added in the
same commit as the test that drives them (FMT-5 discipline).
