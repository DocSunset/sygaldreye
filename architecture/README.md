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
| 10. The Law (needs N1–N8, laws L1–L17, traceability) | 10-laws.md |

Reading order: 0 → 1 → your part → 10. The running example everywhere is
**hello-cosine** (defined in ch. 1 §2). Sources of truth this book distills:
`planning/vision.md`, `adr.md` (ADR-001…012), `planning/lexicon.md`,
`planning/{naming,datasets,trust,bootloader,rhizome,edge_executor.design}.md`.
Where book and planning docs disagree, the lexicon and adr.md govern; fix the
loser in the same commit.
