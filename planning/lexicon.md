# Lexicon — ratified concepts

Status: inventory opened 2026-07-03 (terminology-alignment session). These names
are **ratified as concepts.** One name per concept, one concept per name. This
file remains the ratification LEDGER; full definitions with worked examples
live in `architecture/00-glossary.md` (written 2026-07-03).

## Organizing principle: form vs function (Kevin Austin, via Travis 2026-07-03)

Modular synthesis's form/function distinction at the level of all data and
computation. **Form** (intrinsic): **data — alone** (ratified 2026-07-03: the
form column collapsed to one entry). **The two primal functions**: node (data
read as a container) and link/edge (data read as a pointer); even hash and
route are data functioning as names. Point vs line is perspective, not
structure — everything is material, linked together, each thing a leaf upon
the ground of all being. **All other function** (roles conferred by usage,
patching context, or history): instance, state, ref, dataset, mapping,
executor, input/output/throughpoint, provider/compatible, app/engine/execution,
kind, ground. Rules: (1) never reify a function as a form ("there are no LFOs");
(2) form affords function — the kind system is the catalog of affordances, not a
taxonomy of roles.

## Ratified (Travis, 2026-07-03)

node · graph · edge · stage 0 · compatible · provider · port · input · output ·
throughpoint · derivation · node type · **data**

**data** — the fundamental medium. Bytes stored somewhere, probably meaning
something to someone. Pure form when all function is stripped away. Just bits
somewhere. Traces. (The "just lines" rhizome was looking for. Meaning is never
in it — meaning is a function-relation: data means a kind to a reader who is
compatible with it.)

## Proposed additions (Claude, 2026-07-03 — awaiting ratification)

Naming:      hash · address · route · ref
Roles:       instance · state   (ratified 2026-07-03 — role words, not
             thing-kinds: "instance" names the typed-by relationship; "state"
             names the contents bound at an instance's state link. The clear
             cut = the name cut (hash/route) × the level cut (declaration/
             binding): types declare ABOUT link names one level up; instances
             HOLD the links. Route-named state is historyless by construction;
             committing is opt-in history. Instantiate and tick are the two
             uncommitted derivations — space and time; commit turns either
             into a dataset.)
Data:        dataset · kind · provenance · capture · commit · fork · store
             (derivation refined 2026-07-05, ADR-031: the shape, not the
             act — committed derivation is the recorded case)
Execution:   executor · region · rate · mapping · compile · pass · migration
             ("compile" ratified 2026-07-03, replacing "lowering" — compiler
             jargon that clashed with the codebase's load-bearing "lifting".
             Convention: unqualified compile = graph compilation by an engine
             graph; toolchain use always qualified (cross-compile, C++ compile).
             "elaborate" available as unratified prose.)
Mesh:        peer · mesh · advertisement · capability package
Ground:      native · ground · machinery
Horology:    escapement · movement · crown · complication
             (ratified 2026-07-04, ADR-028: escapement = node contract +
             tick-in-order, the unconditional substrate; movement = a frozen
             realized graph; crown = mutable plan + op applier + op inlet,
             the minimal self-modification; complication = everything else,
             including the liveness organs — present only by choice.)
             (machinery ratified 2026-07-04: the compiled substance that
             realizes function without being addressable — the interior of a
             node/executor/package, reachable only through links, never named,
             never wired. The law: machinery may have ANY interior; its every
             interface is a link. Test: if it has a route or hash, it is not
             machinery, it is a node — at rest machine code is data; running,
             it is machinery. Machinery is what the ontology delegates to
             physics; the ground lives there operationally.)

## Tempting but derived (named by definition when needed, not ratified as primitive)

- type — DISSOLVED (ADR-030): port promises are kind + discipline links
  attached directly to the port declaration; there is no type bundle. "node
  type" survives, ratified, = a kind that carries behavior (an instance's
  `type` link is its kind link wearing the traditional name); bare "type"
  is retired prose.
- document = graph; transclusion = edge whose value is an address
- state = an occurrence's contents; preset = a defaults entity
- pin = provide; archive = a provide-everything peer; sandbox = an advertisement
- tick = an uncommitted derivation; freeze = a committed derivation whose output kind is C++
- subgraph = a node whose contents are a graph
- boot graph = the graph stage 0 ticks first
- entity = node (SUBSUMED 2026-07-03: "entity" was (1) a synonym for node and
  (2) a marker for the content-named half of the name split — but that split
  belongs to names, not things. Two name kinds: hash and route. naming.md's
  "entity name" reads "hash".)
- occurrence = instance (SUBSUMED 2026-07-03: same role, active voice — you
  instantiate a node; matches codebase usage. "Occurrence" survives only in
  prose about text spans if needed.)
- contents = data functioning as a node's substance; bytes = data's unit.
  (SUBSUMED 2026-07-03 by the ratification of data.)

## The link family (ratified 2026-07-03)

**link** = data that refers to other data — the second primal function. The
family members are links distinguished by DERIVED qualities, never declared:
edge = a link functioning as patch wiring · name = a link functioning as
designation (hash = content-derived, cannot lie; local name = conferred by a
container) · route = a composed link (sequence of local names) · address = a
route walked from HERE, the resolver's environment node (ADR-029; root:route
spelling is sugar) · ref = a rebindable link · pointer =
prose synonym, quarantined. Qualities: grounding (derived/conferred),
composition (step/route), rebindability (ref or not), fixity (fixed iff
traverses no ref — the liveness rule restated).

## Open merge questions

- ~~edge vs link~~ RESOLVED 2026-07-03: both live under the link family; link
  is the primal function, edge the wiring role.
- ~~node vs component~~ RESOLVED 2026-07-03: component is NOT a ratified
  concept — imprecise prose, a vestige of the thesis terminology. It survives
  only as repo file-layout jargon (the components/ directory convention);
  the concept is node.
- **port payload kinds vs dataset kinds**: ratified direction is ONE kind system
  (ADR-scale; see 2026-07-02/03 discussion) — lexicon lists only `kind`.
