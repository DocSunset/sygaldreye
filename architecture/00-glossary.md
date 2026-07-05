# Chapter 0 — Glossary

The project's vocabulary, exhaustively, with definitions and worked examples.
Ratification ledger: `planning/lexicon.md` (names were ratified there first;
definitions land here). Organizing principle throughout: **form versus function**
(Kevin Austin) — one form, many roles; never reify a function as a form.

Running example, used in every entry that needs one: **hello-cosine** — a 220 Hz
cosine oscillator, amplitude waved by a 0.5 Hz LFO, into the speakers
(`architecture/01-overview.md` section 2 gives it in full).

---

## Form

**data** — the fundamental medium; the only form. Bytes stored somewhere,
probably meaning something to someone. Just bits somewhere. Traces. Meaning is
never *in* data — meaning is a relation: data means a kind to a reader who is
compatible with it (see *decode*). *Example: the 16 bytes `0x0000dc43...` are
data; only a reader holding the `scalar` kind's decoder sees "220.0".*

## The two primal functions

**node** — data read as a container: a bag of named links, possibly with data
of its own. Everything in the system is a node: graphs, datasets, kinds, types,
programs, provenance records, refs. *Example: `osc0` is a node whose bag holds
`type to osc`, `freq to 220`.*

**link** — data read as a reference: data that refers to other data. The link
family members are links distinguished by DERIVED qualities, never declared:

- **edge** — a link functioning as patch wiring. *Example:
  `osc0/out to vca0/in`.*
- **name** — a link functioning as designation. Two groundings: a **hash**
  (content-derived — computed from its target, so it cannot lie) and a **local
  name** (conferred by a container — meaningful one step at a time). *Example:
  `#b22` is a hash; `osc0` is a local name conferred by a topology node.*
- **route** — a composed link: a sequence of local names walked through
  containers. Routes are keyed by local names, never positions or byte
  offsets — that is what makes them edit-stable. *Example:
  `nodes/osc0/freq`.*
- **address** — a route, walked from *here* (ADR-029): resolution begins at
  the resolver's **environment** node (its wired stores, object store, peer
  table, petnames — astui's ground, per-resolver). What looks like a "root"
  is the first step, answered by an environment container; the `root:route`
  spelling is serialization sugar. *Examples: `#a11/nodes/osc0/out` (fixed);
  `graphs/hello-cosine:nodes/osc0/out` (live, lexical against the wired
  store); `#key-travis/stores/main:graphs/chime` (self-certifying,
  peer-qualified).*
- **ref** — a rebindable link: a node functioning as a mutable name, binding a
  local name to a hash. Its trail of past hashes is undo/history. *Example:
  `graphs/hello-cosine to #a11`, previously ` to  #a10`.*
- **pointer** — prose synonym for link; quarantined, not ratified.

**liveness rule** — per step (ADR-029): a step is *fixed* if and only if its name is
content-derived (a hash — pinned by re-hash verification, whatever container
answered it) or conferred by immutable containment; an address is *fixed* iff
every step is (it denotes one value forever and may be normalized and
memoized), *live* if and only if any step crosses a ref (a subscription: a ref move is
an event that flows to holders). Mutability is a property of the **name**,
not the thing.

## Names, instances, state

**instance** — a role word, not a thing-kind: a node in the kinded-by
relationship — it holds a `type` link to a node type and satisfies that type's
declarations. Route-named, hence editable; created by an edit that links it in,
destroyed by unlinking (existence = being referenced). *Example: `osc0` is an
instance of node type `osc`.*

**state** — a role word: the data bound at an instance's state link.
Route-named state is historyless *by construction* (that's what route-naming
means); committing is opt-in history. Survives graph swaps by migration, keyed
by route. *Example: osc0's phase accumulator.*

**declaration versus binding (the level cut)** — a node type declares *about* link
names, one level up (`ports/freq/kind to scalar`); an instance *holds*
the links (`freq to 220`). Same name, two bags, correlated through the
instance's `type` link — the C++ class/object move, and how PFR endpoint
structs already work.

## Data model

**dataset** — a role conferred by history: a node whose data has been
committed — hash-named, immutable, carrying a provenance header. A dataset
never ticks; its outputs are its data; reading it is reference resolution.

**kind** — the name of a decoding convention: a link to a kind node, which
links to decoder programs (render, traverse, serialize, validate) and
conventions (port-name layout, rate constraints). Interpretation is conferred
by linking context, never intrinsic to bytes. Kind-of-kind is `kind`
(harmless first-order fixed point). ONE kind system spans port payloads and
datasets: a port value is small, transient, uncommitted data of the same kind
family (`scalar`, `audio`, `wav`, `graph`, ...).

**port promises** (formerly "type" — DISSOLVED, ADR-030) — a port declaration
attaches its promises directly as links: `ports/freq/kind to scalar`,
`ports/freq/discipline to value`. The oracle checks (kind, discipline) pairs
at edit time; compiled away at runtime. There is no type bundle and no
freestanding concept named "type"; **node type** survives as the idiom for a
kind that carries behavior.

**derivation** — a committed computation: inputs (hashes) to program to output
dataset, with provenance recorded as a *recipe*. Re-derivable, memoizable
(lookup keyed by input hashes), safe to evict. *Example: compiling
hello-cosine's app graph is a derivation; so is freezing it to C++.*

**capture** — the other commit path: data whose input was the world (a mic, a
hand, a moment). Provenance is *testimony*, not a recipe — which peer (a public
key), what wiring, per-peer wall-clock as description only. Irreplaceable;
provisional until some peer provides it. *Example: recording a take of
hello-cosine's output to a wav dataset.*

**commit** — the act that turns route-named data into a hash-named dataset, by
one of the two paths (derive | capture). A stream cannot be committed —
declaring a stream immutable would be a promise the system can't keep.

**provenance** — the metadata relating a dataset to its origin: recipe or
testimony. Records hashes only (testimony may not shift). **provenance-or-fork**
is the law: every derived thing either tracks its sources or has explicitly,
recordedly detached.

**fork / detachment** — a ref that stops tracking a derivation's output; an
ordinary, recorded rebind. Never silent.

**store** — a graph of dataset nodes, reached only by wiring to it (lexical,
never ambient); plural, nested, scoped. get = wiring; put = a graph edit;
ref = a named node; query = traversal. Never ticks; demand-materialized. The
on-disk object directory is the store package's machinery; the graph is its
face.

**provider / compatible** — availability roles, reusing the capability model.
A peer *provides* a dataset: it holds a copy, keeps it, and advertises it — a
promise it can be fetched here (= pinning). A peer is *compatible* with data it
can make sense of (has the kind's decoders) without promising anything.
Durability = "someone provides it." An *archive* is deployment policy: a peer
configured to provide everything it sees.

**preset** — a defaults node (see *composite graph*) alternative to another;
not a primitive.

## Execution

**node type** — a kind that carries behavior (ADR-030): hash-named,
immutable; declares ports (kind + discipline links) and state schema one
level up; links to its behavior — a graph, or a **native**. An instance's
`type` link is its kind link wearing the traditional name; a data kind is
the behaviorless case of the same mechanism.

**native** — fiat behavior: compiled code registered by linkage. Natives are
the captures of the behavior world — behavior-testimony, where graphs are
behavior-recipes.

**port** — a declared position in a node type's link-name convention: which
names are interface. **input** = a port bound to received influence;
**output** = a port whose value the node produces; **throughpoint** — a port
whose promises are *derived from what it is connected to* rather than declared
(the mark of a mapping).

**rate** — a change discipline on a link: `event` (discrete, must-not-drop),
`frame` (per video frame), `block` (per audio block, hard real-time). Some
kinds constrain rates (audio pins block).

**region** — a maximal subgraph executing synchronously under one scheduler
(same thread, cadence, context). NEVER declared; inferred from port promises on every edit. *Example: hello-cosine's `osc0–vca0–dac0` closure is the block
region; `lfo0` stays frame-side.*

**mapping** — a node whose ports are throughpoints; provides behavior at a
connection: `z-inverse` (cycle), `latch` (frame to block), `snapshot` (block to frame),
`queue` (event across threads), `ring` (stream across threads), `net` (across
peers), `probe` (to observers). Auto-inserted by compilation at inferred
boundaries but always visible and replaceable in the editor.

**executor** — a node owning an inner graph plus the machinery that paces it
(thread, device callback, frame envelope, event loop). Owns pacing and the
platform resource. The recursion point: inside it, the whole
parse/plan/tick story repeats at its cadence.

**tick** (derived) — an uncommitted derivation in time: produce the next data
from linked data. **instantiate** is the uncommitted derivation in space.
Commit turns either into a dataset.

**plan** — the compiled runtime cache of a graph: links resolved to raw
pointers, order fixed. The bag of links is the format; the plan is the cache;
round-tripping between them is the law. Nothing consults kind or promise lookups, or does resolution, at tick time.

**migration** — carrying state across a graph swap, keyed by route (instances)
and lift key (clones). The slot primitive "replace the subgraph of slot X,
migrating state" has two call sites: **exec** (swap your own slot) and
**spawn** (child slot + park with restart policy).

**lift** — stamping a per-cell behavior over a span (one kernel instance per
channel; N clones per excess-rank row), decided during compilation, state keyed
by lift key. UNRELATED to compile (the reason "lowering" was retired).

## Compilation and the tower

**compile** — the graph-level meaning, unqualified: an engine graph derives an
execution graph from an app graph (toolchain use is always qualified:
cross-compile, C++ compile). Deterministic; emits a **compilation map**
(route to route, app instance to execution instance) so state survives
re-compilation.

**app / engine / execution graph** — ROLES in the compilation relationship,
not kinds. Every graph is *defined* (data), *compiled* (by an engine graph),
*realized* (instantiated on hardware). Engine graphs are themselves
defined/compiled/realized: a reflective tower, grounded at stage 0.
**Laziness rule**: the tower is potential, not resident — a level's engine
graph is instantiated only when someone edits at that level.

**pass** — a unit of engine-pipeline behavior. Extension points are PORTS: the
engine graph publishes fan-ins (recognize-region, construct-context,
choose-adapters); packages wire into them. **Order is wiring** — topological
order is submission order, visible as patch structure.

**projection editing** — editing a realized view writes back through the
compilation's inverse route map as an app-graph edit. Compilation inserts
default mappings only where none sits. Conditional-on-compilation desires are
passes (deliberate); refusing write-back is a fork. No shadow override
datasets — definition lives in one place.

**composite graph** — a graph is a composite node: a *topology* node + a
*defaults* node, linked. Defaults are keyed by routes into the topology;
orphaning edits surface as rebase conflicts. Single-file JSON survives as the
interchange form.

**freeze** (derived) — a derivation whose output kind is C++; carries
provenance for unfreezing. Freezing bypasses the bootloader by design.

## Ground and bootstrap

**machinery** — the compiled substance that realizes function without itself
being addressable in the graph: the interior of a node, executor, or package —
the audio device code behind dac, the object directory behind the store, the
thread and callback inside an executor, the trap handler inside a supervisor,
all of stage 0. The law: machinery may have ANY interior, but its every
interface is a link — invisibility is bought with interface discipline. Test:
if it has a route or a hash, it's a node, not machinery (a .so at rest is a
dataset; loaded and running, it is machinery). Machinery is what the ontology
delegates to physics — it is where every ground lives operationally, and it is
the one thing in the system that is neither data nor a role.

**ground** — the role of being where a regress stops, by fiat or testimony.
The pattern appears five times: sacred vocabulary grounds meaning; captures
ground provenance; natives ground behavior; stage 0 grounds the tower; the
header convention grounds decoding. Below stage 0, provenance continues as Nix
derivations; fiat retreats to the toolchain/physics boundary.

**stage 0** — a FROZEN REALIZATION of the boot graph: ordinary (native) nodes
whose parsing, planning, and instantiation happened at build time. The one
irreducible property is pre-existence — something must already be running
before anything can be derived at runtime. Capability manifest: header decode,
parse, registry lookup, naive plan, naive tick, slot swap+migrate, dataset
payload, naive resolver, platform stash. Identical logic on every platform;
never branches on platform. Spawns stage 1 and parks as the fallback that
cannot be edited away.

**boot graph** (derived) — the graph stage 0 ticks first; embedded per target.

**naive resolver** — hash to bytes in the local object directory; grounds
resolution-as-graphs; independently invokable forever (debugger of last
resort).

**registry** — a frozen store-graph of node types present by linkage
(CMake-generated per-target registration TU; loud link failure); readable as
the palette.

## Mesh and trust

**peer** — a process or machine participating in the mesh; sovereign over
three published lists: what it will run (advertised node types), serve
(provided datasets), and subscribe to.

**mesh** — the set of peers that accept each other's keys; a security boundary
(capability placement makes graphs remote code execution by design).

**advertisement** — the permission system: a peer only instantiates node types
it advertises; placement is pull-shaped (nothing can be pushed onto a peer,
only requested of it). Selective advertisement IS the sandbox.

**capability package** — an executor flavor as a world apart: vocabulary
(platform node types) + passes (wired into engine fan-ins) + machinery (C++
context internals). Splicing one in is an ordinary engine-graph edit driven by
environment observation. When absent locally, compilation may place the region
on a remote peer advertising the capability.

**graphs versus plugins** — two speech acts: a graph arriving over the mesh is
bounded composition of what the receiver advertised; a native plugin is new
capability injection, gated by provenance policy (signed, chain-of-derivation).

**decode** (derived) — derive in the reading direction: apply a kind's decoder
to data, producing a view. Reading is derivation; there is no plaintext (only
widely-shared decoders); secrecy is decoder scarcity. Every chain of decoding
terminates in someone's senses (screen, dac — the last decoders).

## Documents (the rhizome layer)

**document** (derived) — a graph; **transclusion** (derived) — an edge whose
value is an address (fixed = quotation, live = subscription). The editor is a
document browser; the store browser inherits the astui navigation model:
*here* (a single node), *path* (breadcrumb history), *frontier* (what's
reachable, recomputed each step), *mark* (the authoring primitive — the marked
subgraph is the durable map).

**round-trip metric** — validation target inherited from the rhizome probe:
decompose C++ (or any corpus) into the document form by
transclusion-to-the-limit and regenerate it byte-identically; every permissive
codebase is a unit test for the medium.

## The greenfield stratum (ADR-013 through 028, 2026-07-03/04)

**escapement** — the node contract + tick-in-order: the only unconditional
substrate; a calling convention and a for-loop. **movement** — a frozen,
flattened, realized graph the escapement ticks; a sealed firmware is
escapement + movement. **crown** — the minimal self-modification: the plan
as mutable data + one op-applier primitive (at tick boundaries) + an op
inlet. **complication** — everything else, including the liveness organs
(parser, codec, resolver, registry-face, slot, subgraph, ref, reflection,
query four, the seven mapping definitions): core-named where uncomposable,
present only by tape choice. **boot tape** — the boot graph as flat op
records replayed by the crown (a graph is equivalent to its building ops). **per-sample
island** — a cycle executed sample-interleaved; default z-inverse = one sample
(ADR-013). **discipline** — one of the three closed semantic categories
(event push, value dirty-push/demand-pull, stream clocked); cadence is a
clock, and clocks are open executor outputs (ADR-015 and 020). **determinism
class** — a derivation's reproducibility promise: exact, platform-exact,
approximate, nondeterministic (ADR-021). **op log / op tree** — history as
attributed, inverse-carrying edit ops; gestures are transactions; the tree
never discards; linearity is a view (ADR-018). **lock** — a graph's
vocabulary map (name to type CID); upgrades are one link swap (ADR-026).
**succession** — how anything evolves: nodes are succeeded, never mutated;
migrations are lazy derivations; compatibility is reachability (ADR-025).
**arbiter** — the one op queue per live instance; arrival order is the
order (ADR-023). **quarantine tier** — fresh plugins realize in subprocess
regions until trust promotes them (ADR-016). **conformance profiles** —
movement-level (behavioral) and peer-level (protocol); the suite is the
system's definition, written in the system (ADR-026 and 027).

## Retired / quarantined prose

**entity** = node, **occurrence** = instance, **contents** = data as a
node's substance, **component** = unratified thesis vestige (survives as the
`components/` directory convention), **lowering** = compile, **pin** =
provide, **archive** = provide-everything policy, **sandbox** =
advertisement, **pointer** = link.
