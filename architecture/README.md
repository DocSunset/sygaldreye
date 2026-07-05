# The Sygaldreye Architecture Book

Written 2026-07-03 from the ratified outcomes of the 07-02/03 architecture and
terminology sessions. For implementers and maintainers — including builder
agents: each part chapter is a self-contained handoff with enumerated
requirements (`XXX-n`) and acceptance criteria (`XXX-n.m`) written to become
automated tests.

| Chapter | File |
|---|---|
| 0. Glossary | 00-glossary.md |
| 1. Architectural overview | 01-overview.md |
| 2. Data, naming, kinds, types (NAM) | 02-data-naming-kinds.md |
| 3. Store, commit, availability (STO) | 03-store.md |
| 4. Execution (EXE) | 04-execution.md |
| 5. Compilation & the tower (CMP) | 05-compilation.md |
| 6. Stage 0 & boot (SZ) | 06-stage0-boot.md |
| 7. Capability packages (PKG) | 07-capability-packages.md |
| 8. Mesh & trust (MSH) | 08-mesh-trust.md |
| 9. Editor & documents (EDR) | 09-editor-documents.md |
| 10. The Law (needs N1–N8, laws, traceability) | 10-laws.md |
| 11. The language core (LNG) | 11-language-core.md |
| 12. Node authoring & conformability (AUT) | 12-authoring-conformability.md |
| 13. The native contract (ABI) | 13-native-contract.md |
| 14. Formats & wire protocols (FMT) | 14-formats-protocols.md |
| 15. Time, concurrency, memory, faults (TCF) | 15-time-concurrency-faults.md |
| 16. The Core (COR) — escapement · crown · complications | 16-the-core.md |
| 17. Conformance & evolution (CNF) | 17-conformance-evolution.md |
| A. The greenfield build order | 18-appendix-greenfield.md |
| B. Genealogy — ancestors, organs, and the vocabulary map | 19-appendix-genealogy.md |

The book was consolidated 2026-07-04: it is now the SOLE design document —
the pre-book planning docs were folded in and removed (git history keeps
them). ADR-013 through 028 (the greenfield session) govern; where an older chapter
sentence and an ADR disagree, the ADR wins and the sentence is a bug. The
probe under `probe/` is a deprecated design probe: reference and
salvage (see the appendix), never migrated.

Reading order: 0 to 1 to your part to 10. The running example everywhere is
**hello-cosine** (defined in ch. 1 section 2). Sources of truth this book distills:
`planning/vision.md`, `adr.md` (ADR-001 through 012), `planning/lexicon.md`,
`planning/{naming,datasets,trust,bootloader,rhizome,edge_executor.design}.md`.
Where book and planning docs disagree, the lexicon and adr.md govern; fix the
loser in the same commit.
