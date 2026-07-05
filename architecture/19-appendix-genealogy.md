# Appendix B — Genealogy

*Every organ in this design was transplanted from an ancestor that got one
thing right, at the seams where the ancestors agree. The two welding points
that make the transplants compatible are immutability (values by hash) and
links (everything reaches everything by name). This appendix records, per
ancestor: what we took, what we refused, and why — then maps the vocabulary.*

## The instrument ancestors

**Modular synthesis (Doepfer, VCV; Kevin Austin's pedagogy).** Took: the
form/function distinction raised to the level of all data and computation —
one form family, meaning from patching, "there are no LFOs"; the ethos that
the boundary between playing and building should not exist. Refused: the
implicit summing of merged cables (summing is an `add` node, explicitly).

**Pure Data / Max/MSP.** Took: the patch as program; hot-vs-cold and
control-vs-signal — *as problems to dissolve*: our rate-keyed disciplines
make push/pull a property of port kinds instead of per-inlet folklore;
`block~ 1` and gen~ became per-sample islands, scoped automatically. Took
also: abstractions-with-creation-args (subgraph defaults). Refused:
right-to-left evaluation order (order is wiring), hidden boundaries (ours
are visible mappings), the rigid control/signal wall (regions are inferred).

**Faust / gen~.** Took: per-sample kernel fusion, feedback as loop-carried
variables, the conviction that a patch can compile to austere portable code
(the freezer, movements). Refused: freezing as a different *language* — ours
is a backend of the same compilation, pure optimizer by law (ADR-013 and 014).

**APL.** Took: rank polymorphism — conformability lifting; an N-by-3 span into
a vec3 port stamps N instances the way APL lifts a scalar function over an
array. Refused: the implicit everything — lifts are visible in the plan and
keyed for identity.

**libmapper / the Sygaldry thesis.** Took: the mesh of named signal
endpoints discovered and mapped across devices; the endpoint/throughpoint
vocabulary itself. Refused: mapping as a layer *outside* the program — here
the mesh is the program's own fabric.

## The systems ancestors

**Unix / Plan 9.** Took: the ontology — everything is a file roughly equals everything is
a node; small tools composed roughly equals small nodes wired; the calling convention as
the only universal (the escapement). Refused: the semantics — in-place
mutation, no provenance, untyped bytes with conventions held in folklore.
We kept the shape and repaired the physics.

**git.** Took: immutable content-addressed objects + mutable refs; history
as hash-chains; canonical encoding hashed, packfiles as caches
(format-is-law); `hash-object` on a ref file being legal and inert. Refused:
bare hashes (no self-description — the SHA-1 migration wound), positional
links, four fixed shapes; and commits-as-snapshots (our history is the op
tree; snapshots are checkpoints).

**Nix.** Took: derivations — recipes over content-addressed inputs;
memoization, substituters (provide/compatible), GC-by-roots thinking,
the flake lock (vocabulary locks), and the ground itself: below stage 0,
provenance continues as Nix derivations. Refused: reference-by-grep (links
are first-class values), and the store as ambient singleton (stores are
graphs, reached by wiring).

**IPFS / multiformats / IPLD.** Took: CIDs (self-describing hashes),
dag-cbor (determinism by spec, not by single implementation), Merkle
chunking, pin/cache semantics. Refused, at household scale: the DHT,
Bitswap, IPNS — global trustless machinery where five paired peers need a
question asked out loud; reinstated as the transport tier at world scale.

**Build systems à la carte / self-adjusting computation / FRP /
spreadsheets.** Took: the reactive core — dirty-push + demand-pull,
incremental recompute as the semantics of the value discipline; the
spreadsheet's deepest gift is *projection editing*: you edit the cell you
can see and the source is what changes. Refused: spreadsheet anonymity —
every cell here has a name, a kind, and a provenance.

**Erlang/OTP.** Took: supervision — spawn-and-park, restart intensity
limits, let-it-crash recast as "failure is death, errors are values";
process isolation as a placement choice. Refused: mailboxes as the
computation model (we wire; we don't send).

**Forth / uxn / PICO-8.** Took: the bootstrap aesthetic — a tiny fixed
inner loop plus a dictionary the system extends at runtime (the crown and
its tape are Forth's outer interpreter and `,` wearing horology); sealed
tiny artifacts as first-class citizens (a movement is a cartridge).
Refused: the untyped stack — our ops carry preconditions and inverses.

**Smalltalk / Self / Pharo (and JavaScript's prototypes).** Took:
uniformity and liveness — everything an object/node, the system editable
while running, classification by delegation link, roles over classes.
Refused: the engine — encapsulated mutable state (identity by allocation,
aliased mutation) and synchronous message dispatch. We kept the chassis and
swapped in dataflow over immutable values. The characteristic bugs change,
which is how you know the paradigm did.

**Reflective towers (3-Lisp, Smith).** Took: the tower — every level
defined, compiled, realized, including the compiler; grounding at a fixed
base; the laziness rule (meta-levels are potential, not resident). Refused:
reflection as *semantic* self-modification mid-step — our tower moves only
by ops and derivations, recorded.

**Xanadu.** Took: transclusion; links attached to permanent identity so
they survive editing; the enfilade problem (sequence-kind traversal);
bidirectional links — made honest by bounding them to the reachable trust
domain. Refused: the global tumbler address space (content hashing needs no
allocator) and the sacred view (exactly one projection is authoritative for
identity; all others are cheap and plural).

**The Web.** Took: the sandbox as the reason strangers' code is runnable —
generalized into advertisement (declarative, per-relationship); URLs —
generalized into addresses-as-routes; view-source — deepened into
provenance. Refused: the server/client asymmetry (peers), the DOM as the
one projection, and links that rot (hashes verify; refs are signed).

**SSB / hypercore / IPNS / petnames.** Took: self-certifying mutable
namespaces — a peer's key IS its root authority; signed ref-states;
petnames instead of global registries. Refused: append-only-log worship —
our logs are trees, and forks are a feature with a law.

**Game servers / CRDTs.** Took (game servers): the arbiter — one queue,
arrival order, attributed ops, for the live case. Refused (CRDTs, from the
core): convergence-eventually as a promise for live instruments; silent
convergence is silent semantic surprise. Admitted per-kind, behind links.

**Horology.** Took: the entire nomenclature of the floor — escapement,
movement, crown, complication — because watchmaking already named the
minimal mechanism, the frozen work, the one adjustment interface, and the
optional everything-else, and named them with the right humility.

## The vocabulary map

| sygaldreye | nearest ancestral term(s) | what changed |
|---|---|---|
| data | Unix bytes; Shannon's signal | meaning explicitly conferred by decoding, never assumed |
| node | file/dir; Smalltalk object; Pd object box | no encapsulated mutation; a bag of links, hashable at rest |
| link | pointer; symlink; URL; Xanadu link | first-class, typed by derived qualities; survives editing via hashes |
| hash | git oid; CID | self-describing (multiformats); verified, never trusted |
| ref | git branch; IPNS name; symlink | single-writer, signed states, trail-as-undo |
| address | URL; file path; JSON pointer | a route walked from the resolver's environment (no global root) |
| store | .git objects; /nix/store; IPFS repo | a graph, plural and scoped, reached by wiring — never ambient |
| dataset | git blob/tree; Nix store path | + kind + provenance header; a node playing a committed role |
| derivation / recipe | Nix derivation; make rule | + determinism classes; memo keys honest about floats |
| capture | git blob you committed; a take | testimony (signed peer, wiring); the irreplaceable ground |
| commit | git commit | folds an op range; two doors (derive/capture) |
| memoization | Nix substitution; ccache; build cache | mesh-wide, keyed by input hashes + class fingerprint |
| kind | MIME type; prototype; class | a node carrying decoders; one system for wires and store |
| node type | class; Pd object class; VST | a kind that carries behavior; succeeded, never versioned-in-place |
| instance | object; Pd box; voice | route-named; state migrates by route across swaps |
| port promises | type signature; Pd inlet type | two links (kind, discipline); checked at edit time, erased at runtime |
| discipline | control vs signal (Pd/Max) | three forever: event push · value pull · stream clocked |
| clock | audio callback; rAF; MIDI clock | an executor output, wired like any input; open set |
| region | gen~ patcher; block~ subpatch; audio thread | inferred, never declared; recomputed per edit |
| mapping | implicit conversion; adapter | reified, visible, replaceable on the canvas |
| per-sample island | block~ 1; gen~ | scoped automatically to the cycle; freezer fuses it |
| plan | bytecode; packfile; prepared statement | a disposable cache of the graph; round-trip is law |
| compile | lowering; elaboration | deterministic derivation emitting a route map; memoized |
| freeze / movement | Faust → C++; PICO-8 cart; uxn rom | a realize backend; pure optimizer; provenance for unfreezing |
| escapement | Forth inner interpreter; uxn core; libc's crt0 | node contract + tick; the only unconditional substrate |
| crown | Forth outer interpreter + `,`; REPL | mutable plan + op applier + inlet; ops at tick boundaries |
| boot tape | Forth boot block; init script | fixed-format op records; a graph ≡ its building ops |
| complication | library; package; plugin ecosystem | includes the bootloader's own organs; chosen, never imposed |
| op log / op tree | git reflog; event sourcing; vim undo-tree | inverses + preconditions + attribution; linearity is a view |
| arbiter | game server; Raft leader (degenerate) | per live instance; arrival order is the order |
| fork / succession | git branch/fork; semver major | recorded detachment; migrations are lazy derivations |
| vocabulary lock | flake.lock; package-lock.json | per-graph name→kind-CID map; upgrade = one link swap |
| provide / compatible | IPFS pin; Nix substituter | the capability model reused for data; durability = someone provides |
| advertisement | browser sandbox; capability list; CORS | the permission system; placement is pull-shaped |
| peer / mesh | host; federation; Link session | keys as identity; graded circles at scale |
| transclusion | Xanadu transclusion; iframe; #include | an address-valued edge; fixed = quotation, live = subscription |
| projection | view; serialization; pretty-printer | open-ended decoders; exactly one is authoritative for identity |
| editing surface | spreadsheet cell; direct manipulation | a projection with an inverse map (write-back through routes) |
| query four | SQL/datalog; find(1) | graphs, not syntax; standing queries are reactive values |
| lift / span | APL rank; SIMD; instancing | visible in the plan; keyed identity survives resize |
| supervision | OTP supervisor tree | spawn-and-park; policy is wiring; testimony on death |
| ground | axioms; /; the metal | named role: where each regress stops, by fiat or testimony |
| machinery | runtime; VM internals; driver | any interior, every interface a link; what physics gets |
| conformance suite | test suite; TCK; spec | IS the system's definition; written in the system; blessing is testimony |

*Reading the table columnwise is the argument of this appendix: the left
column is one system; the middle column is a dozen; the right column is the
same two moves applied everywhere — give it a verified name, and make its
boundary a visible link.*
