Be brief. Less is more. Say less.

Write less code. The best code is no code. Every line has a cost.

Make graphs, not C++. If it cannot be a graph yet, the missing vocabulary
is the bug: build the nodes that make it possible, then build it as a
graph. (L22 — a guiding star, equal in rank to no-restarts.)

Treat all warnings as errors, even if the compiler or linter does not. Fix them unless you're prepared to argue there's a good reason not to, or unless they're out of scope for your current task, in which case add a bug work item to `kanban/backlog`

# Project Standard

- The basic unit of implementation is a software component.
- A software component is embodied by its documents: design documentation, production source code, automated tests, and build automation scripts.
    - Component names use `lower_snake_case`.
    - Design documents are called `component_name.design.md`
    - Source code documents use the extensions appropriate for the source code (e.g. `.hpp`, `.cpp`)
        - Each `.cpp` file should almost always be less than 100 lines of substantive code, not including `#include`s, copyright boilerplate, and trivial code that is understood at a glance without even reading it..
        - Each `hpp/cpp` pair should define only one substantive software component (e.g. not two complex namespace-scope classes)
    - Automated tests are called `component_name.test.xxx` using the extension appropriate for the source code
    - We use cmake for build automation
    - All documents should be placed in a directory whose name is the name of the component
- Design documentation must document the following design features:
    - Ports
        - Inputs: Places where influence comes into the component that semantically belong to the component and do not depend on how other components behave
        - Outputs: Places where influence flows from the component that semantically belong to the component and do not depend on how other components behave
        - Sources: Places where influence comes into the component that depend on the behavior of another component; they are coupled inputs
        - Destinations: Places where influence flows from the component that must meet the needs of another component; they are coupled outputs
        - Temportal couplings: Places where the behavior of the component is coupled to another component's events happening in a certain order
        - Intended seams: Places where the behavior of the component is intended to be extended without modifying the component's own source code, e.g. for testing or derived components
    - Requirements (e.g. functional and non-functional and so on)
    - Allowed dependencies: an explicit list of names of components this one is allowed to depend on
    - Owning package: the name of the package to which the component belongs
- Packages are groups of components
- A package should have a `package_name.design.md` document that lists the other packages that components in the package are allowed to depend on.

# Tooling (probe-scoped — run from inside `probe/`)

- Prefer Nix as package manager for bringing in external dependencies
- `probe/sh/build.sh [--clean]`: build the probe
- `probe/sh/format.sh [--check]`: auto-formatter
- `probe/sh/lint.sh`: static analyzers
- `probe/sh/test.sh`: build test binaries, push to a connected Quest 3 via adb, and run them on-device
- `probe/sh/run.sh [args]`: run the probe interactively

## Testing

- Use **Google Test** (sourced from `$ANDROID_NDK_ROOT/sources/third_party/googletest`) for all component test executables.
- Test files are named `component_name.test.cpp` and live in the component directory.
- Build gtest directly from its `.cc` sources; do not use its CMakeLists.
- Tests run on-device via adb, not on the host.

# Goal

Sygaldreye: a protocol for encoding recursive metadata and decoding data to
derive other data — grounded in stage 0, paced by executors, shared by a
mesh, terminating in someone's senses. Near-term: a live-patchable
audio/visual/XR environment across Linux host, Quest 3, and browser peers.
Far-term: the hypermedium (see `planning/vision.md`).

**Building the greenfield? A capable agent starts at `HANDOFF.md`;
everyone reads `BUILDER.md`; then run
`python3 conformance/run.py` — its output is the to-do list.**

# Architecture — where truth lives

- **`architecture/` — the book.** The sole design document: glossary,
  overview, part chapters with enumerated requirements and acceptance
  criteria, the laws, the greenfield build order. Read its README first.
- **`adr.md`** — ratified decisions (ADR-001…005 are probe-era platform
  choices; ADR-006…028 are the current design). Architectural evolution is
  recorded there and reflected in the book in the same commit.
- **`planning/lexicon.md`** — the terminology ledger (one name per concept;
  form vs function; retired prose is binding).
- **`probe/` is the DEPRECATED DESIGN PROBE** (the entire pre-greenfield
  implementation: components, apps, scripts, assets, its flake): reference
  and salvage (see the book's appendix), never migrated. Its bug cards in
  `kanban/` are probe-scoped. Build/run it from inside `probe/`.

# Workflow

Work items are located in the `kanban` directory, one markdown file per work item. Move items to develop when you are developing them. If the item requires human review, move it to review when it is done. If it does not require human participation in the review process, move it to done when you are done working on it.

# Continuity (context is a cache)

The repo is the memory; your context window is a cache of it and must
round-trip. After any compaction, and whenever in doubt, the resume
protocol is: re-read `HANDOFF.md` and `BUILDER.md` in full (the Judgement
section is binding), then `planning/status.md`, then run
`python3 conformance/run.py`. Keep status.md's TOP section a resume block
— current rung, in-flight criterion, next action, active disciplines —
updated at every stopping point. Anything load-bearing that exists only in
conversation is already lost; write it down before it matters.

# Remember

Be brief. Less is more. Say less.

Write less code. The best code is no code. Every line has a cost.

Make graphs, not C++. If it cannot be a graph yet, build the nodes that
make it possible — then build it as a graph.
