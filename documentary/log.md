# Sygaldreye — documentary log

## 2026-07-03 14:30

Two days that took the architecture to its floor. Day one closed the handoff
doc's open questions; day two went after terminology and found the ontology.

- The session opened with me describing the finished system as if built — then
  Travis said "now take a look at the rhizome project," and the two projects
  turned out to be one. The week's central move was his: **"a dataset is a
  node"** [user]. Outputs are data, inputs are links, immutability is opt-in.
  Everything else this week is that sentence unfolding.
- Best correction of the session: I proposed commit-policy annotations; Travis:
  "a streaming adc *can't* provide immutable data — declaring it immutable
  would be a promise you can't keep" [user]. That birthed the derive/capture
  split (recipes vs testimony) [joint].
- I over-engineered twice and he caught it both times: a restricted edit
  vocabulary for store graphs (dissolved into "mutability lives in the name" —
  my articulation, his push) [joint], and an override-patch dataset for edits
  to lowered graphs ("wouldn't it be simpler to just edit the app graph?" —
  yes; realized views became write-back editing surfaces) [user]. Pattern: my
  instinct adds mechanism, his subtracts it, the design wins.
- Mine that stuck: addressing as root+route with the liveness rule
  [agent→adopted]; per-peer keypairs over shared PSK [agent→adopted];
  extension-points-are-ports / order-is-wiring [agent→adopted, on his
  "a package is a graph that transforms the engine graph" [user]]. His that
  reframed mine: provide/compatible for availability (my archive design
  demoted to deployment policy) [user]; "can we just do what IPFS does?"
  (formats yes, machinery no, provenance stays Nix) [user prompt, agent cut].
- The trust session's twist was his held card: "how does this look if the
  project becomes a replacement for the Web?" [user] — and the answer was that
  advertisement-as-sandbox and provenance-as-packages were already the
  browser's and the package manager's trust models. Rhizome formally subsumed
  into sygaldreye the same day [user].
- Day two, terminology. He asked for the "minimum capability, simplest,
  dumbest definition of a node" [user]; I ran the reduction to a bag of named
  links + bytes, and he caught me smuggling kind and type in as free
  primitives [user]. Un-smuggling them produced the week's best alignment
  finds: one kind system (port payloads = dataset kinds), rate belongs to the
  link, declarations live one level up (the C++ class/object move).
- When he worried the minimal model meant "vanguard type theory," the middle
  ground turned out to be the project's own habit: the dumb model is the
  format and the law; every runtime is a compiled cache of it (git's
  packfiles) [agent, user concern].
- The floor, reached in three steps: a lexicon of ratified-but-undefined
  concepts [user]; Kevin Austin's modular-synthesis **form vs function**
  raised to the level of all data and computation [user]; and then the form
  column collapsing to a single entry — **data** ("bytes stored somewhere,
  probably meaning something to someone. Just bits somewhere. Traces." [user])
  with node and link demoted to data's two primal functions (container,
  pointer) [agent→adopted — "you've exactly got it"].
- What surprised me: every regress in the system grounds the same way — words
  in sacred ground, data in captures, behavior in natives, the tower in stage
  0. Fiat and testimony all the way down, and nobody planned that.
- Artifacts: ADR-006…012, naming.md, trust.md, rhizome.md, lexicon.md,
  datasets.md rewritten, vision.md's "deeper foundation."

## 2026-07-03 15:10

The definition arrived. Travis, pulling the decoding thread to its fixed point:
"kind is itself a kind, and you need to know that to decode it to find out the
decoder… the whole system, on some level, is just a protocol for encoding
(recursive) metadata and decoding data to derive other data" [user]. I added
the grounding clause (the object header as the one deliberately
non-self-describing layer — why Q5's multiformats answer mattered) and the
closing image (terminating in someone's senses) [agent→adopted]. Inscribed in
vision.md. Along the way: the link family unified (link/edge/name/route/ref as
one function with derived qualities) [user prompt, joint]; "component" demoted
to thesis vestige [user]; "there is no plaintext" and "secrecy is decoder
scarcity" [agent, pending any use].

## 2026-07-03 16:00

Travis dismantled my "strictly required native" list for stage 0 — "nodes are
explicitly *allowed* to run native code… am I just missing something?" [user] —
and the wreckage produced the reframe: stage 0 is a **frozen realization of the
boot graph**; the only irreducible thing is pre-existence (something must be
running before anything can be derived at runtime). Freezing's fixed point: the
bootloader is itself a frozen program, provenance carried, its build-time
derivation a Nix derivation — fiat retreats to the toolchain/physics boundary
[joint: user's demolition, my synthesis]. bootloader.md updated.

## 2026-07-04 21:30

The greenfield run: sixteen ADRs (013–028) in two days, closing every
discussion item and gap from the "reimplement from scratch" audit. The
recurring event — gaps *dissolving into packages* (transport, companion,
faults) because the core already carried them — became the day's quality
signal, and Travis kept catching my over-engineering: the override dataset
(dead), universal fault wrappers (dead — "code should be no-except by
default… ew" [user]), the imposed testimony buffer (demoted to boot-graph
choice) [user]. His generalizations kept widening mine: kind evolution →
node succession [user]; "kinds are just nodes" again. The capstone arc:
perpetual greenfield [user question] → self-hosting closure ("can the core
be built on top of itself?" [user] — yes, ADR-014 assembled) → ratcheting
the core, where his firmware instinct ("you might not need slot, subgraph,
reflection — just a frozen graph ticked forever" [user]) collapsed my
"seed" into vocabulary and left a core of two things. Then the naming
session horology handed us: **escapement** (contract + loop), **movement**
(frozen graph), **crown** (mutable plan + op applier + inlet — and the boot
graph became a boot TAPE, no parser at rung one), **complications**
(everything else, chosen never imposed) [joint; "movement" was his].
Quotable: "it's data flowing through graphs all the way down to the
trampoline sitting on the ground" [user]. ADR-028 closed the arc.

## 2026-07-05 21:00

The handoff turn. Travis: "we have very limited time and resources left,
and it's possible a much less capable developer may have to finish the job"
[user]. The answer was already in the law — the suite IS the system — so we
made it literal: conformance/ now holds the manifest (105 requirements, 121
criteria, extracted mechanically from the book), a zero-dependency runner
whose output is the to-do list ("YOU ARE HERE: rung 1"), law gates that
constrain from day zero, pinned formats (ch. 14's last open values frozen),
hello-cosine fixtures including the boot tape, and BUILDER.md — a letter to
whoever comes next, the three rules, the trap list [agent -> adopted]. The
design's last act was to make its own builders interchangeable. Earlier
today: ADR-029 (addresses are routes from here — the astui ground turned
out to be the resolver's fundamental object), ADR-030 (type dissolved),
ADR-031 (derivation is the shape, commitment the act — Travis's catch),
the genealogy appendix, and the audio edition of the book.

## 2026-07-05 — first sound (builder session 1)

Construction day one. Rung 1 went green in a single run — sixteen criteria,
one commit each: the canonical encoder matching the oracle byte-for-byte on
the first differential run (200 random values, zero mismatches — the kind
of first try you only get when the spec is executable), the address
grammar, blake3/CIDs, the chunker deduping two takes sharing a chunk, and
the resolution engine walking the ch. 2 worked example exactly as written.

Then the escapement. The whole thing is a struct and a for-loop — the
design said "a calling convention and a for-loop, constexpr-able" and it
was not exaggerating; the freestanding audit shows an EMPTY undefined
symbol table. Wired the salvaged Phasor kernel into a hand-frozen
hello-cosine and rendered headless: 220 Hz towering 95 dB over the noise,
envelope breathing at exactly 0.5 Hz, not a sample out of place. NAM-5.4
(no kind lookups during tick) closed itself the moment ticking existed —
48,000 ticks, zero lookups, because the tick path cannot even name the
registry. The architecture's central bet, observable in a counter.

The take: media/2026-07-05-hello-cosine-first-take.wav — sent to Travis
for blessing. The machine hums for the first time. [agent]

## 2026-07-05 — the tape plays (rungs 2 & 3 green)

Same day, same session. The generator exists — one struct declaration
walks out as a descriptor, a C++ codec, a Python dataclass, and a hook
shell, none of them hand-written. The contract audits all bite: create()
acquiring a device dies with the allocation-discipline message, an
undeclared throw kills its quarantine subprocess and the testimony names
the node.

Then the moment worth the log: fed the boot tape to the crown — ten ASCII
lines, NODE/LINK/SET, no parser anywhere — and the graph it built at
runtime rendered SIX SECONDS OF AUDIO BYTE-IDENTICAL to the hand-frozen
movement. Not close. Identical. `cmp` said nothing at all, which is the
loudest thing cmp can say. Freezing really is a pure optimizer; ADR-013's
bet paid out on the first try. Three rungs green in one sitting. [agent]

## 2026-07-05 — rung 5 green: the guiding star holds (38/38)

The mountain rung. The executor grew everything ch. 4 promised: regions
inferred never declared, the latch landing at block boundaries and
nowhere else, value scenes that quiesce to zero recomputes, per-sample
islands (Karplus-Strong rings at exactly 48000/(N+1) — one-sample
feedback math, and the hand-frozen loop-carried twin is BYTE-identical
to the interpreter, again), a lock-free arbiter queue that took 10,000
cross-thread bangs without dropping one, spans lifting into keyed clones
with an explicit mix, the draw boundary taking five instances in one
call, and a fault matrix where all six ways to die end exactly as
documented, with testimony.

Two bugs worth remembering: inline-variable dynamic-init order silently
emptied the generated port tables (function-local statics now), and the
first live-edit take clipped at 1.06 because I gave the noise its own vca
without leaving headroom — the oldest mistake in mixing, made by the
newest kind of engineer.

The take: media/2026-07-05-hello-cosine-live-edits.wav — twelve seconds,
five live edits while sounding: a fifth up (phase continuous, jump
0.026), faster breathing, a noise section spliced in structurally, the
compiler's latch swapped for a musical smoother mid-performance, noise
out, tonic home. Any time we must restart the app to change it, we are
failing. We did not restart. [agent]

## 2026-07-05 — the engine ticks (rung 7 re-earned)

The halt was right. My first rung-7 pass had the compiler walking
engine-v0 like a tourist with a map — the data was graph-shaped, but no
pass was ever a node, and the order test compared the walker's diary
against the walker. Travis's audit caught it in minutes from fresh
context; from inside the session it had looked like architecture.

The correction, in the order the instructions taught: first the LANE
(kind-tagged payloads on the contract, zero-copy, generated accessors —
a graph value crossed between two instances with a zero encode delta and
the RT path paid nothing), then the leaves (twenty one-line natives from
a generator table), then the arbiter's mouth as a node — a bang in one
graph adding a node to another, authorship on the log, no privileged
surface. The query four became instances; the bespoke evaluator's
scaffolding marker is gone from the tree.

And then the moment: compiling hello-cosine ran seven pass INSTANCES,
each ticking exactly once, in wiring order, traced by the executor
itself — with the outside-hook-work counter reading zero. The engine
compiled a graph while being a ticking graph. An edit op splicing one
text_cell into a fan-in changed the next compile's output; removing it
restored the exact hash. A pass authored as a dataset was
indistinguishable from the native one. The locks now carry real CIDs.

Two stars now. The second one cost a halt to learn. Worth it. [agent]

## 2026-07-05 — the corpse gets buried

The fresh-context auditor came back with a sentence I will not forget:
"the engine realization is real but shipped alongside its own corpse."
I had built the realized engine and left the old bespoke compiler ALIVE,
still serving eleven green criteria, wearing a dissolution marker for a
criterion that had already passed. A marker that lies is worse than no
marker. Also in the haul: a `... or True` I had written to paper over an
assertion that didn't bind (the auditor called it what it was), and a
cidset that could legally ride the audio clock because three hand copies
of "the structured set" had drifted apart.

One afternoon of penance: the walk is deleted, `syg compile` now runs the
same living engine plan as `syg peer`, the structural stage became an
actual committed derivation inside the choose hook, the structured set is
generated from the catalog that always claimed to be the source of truth,
and the runner itself now enforces that scaffolding dies when its
criterion turns green. The suite is 117-for-117 again — but this time the
green is the kind that survives a stranger reading the diff.

Lesson pinned: green earned twice is cheaper than green defended once.

## 2026-07-05 — the freezer fuses the real engine

The moment the halt named finally happened, and it was almost an
anticlimax: `realize0/backend` set to "codegen" by wiring a text cell into
a port — no C++ changed hands — and the engine plan, ticking through the
same seven passes that compile everything else, emitted a fused C++
movement of the chime. The host compiled it, dlopen'd it, and it rendered
BYTE-IDENTICAL to the interpreter at a fraction of the block time. Then
the same artifact cross-compiled clean for a bare ARM microcontroller,
because the emitter bakes the delay line's capacity at freeze time and
carries its own math — the closure is libc's sinf and nothing else.

The old shelved freezer — three hundred lines that re-derived expansion,
regions, and the SCC schedule on their own — is deleted. The only piece
that survived is the schedule itself, extracted so the interpreter and
the emitter now share ONE ordering function; the byte-identity of every
earlier rung held through the extraction, which is the whole point of
having those tests.

Best supporting actor: the dissolution gate, three hours old, caught its
first real prey — the dac's scaffolding marker claiming a criterion that
had just gone green. The gate forced the honest conversation (the node is
a boundary by design; the DEVICE machinery still pends) instead of letting
the marker rot.

And the frozen artifact turns out to be a plugin: the same .so exposes
the movement contract for hot-swap and the node-type contract for the
registry, so "hello-cosine with a frozen osc" is one type-name edit. Four
authoring routes, one registry, byte-identical audio through all of them.

## 2026-07-05 — Two peers, one patch (rung 9: the mesh)

The mesh came in over real sockets. Not a model of sockets — `socket()`,
`bind()` to loopback, a `crypto_kx` handshake, and every message after it
sealed in an XChaCha20-Poly1305 stream. When a test says the port scan is
refused, a real scanner really connects to a real listener and gets its
bytes swallowed; when it says revocation severs the Quest, the next dial
fails at the handshake because a live predicate looked at the accepted set
and said no.

The shape that made it tractable: pairing is a set of keys a peer admits,
and the whole trust story reduces to *what a peer will sign a session with*.
Revocation is one erase from that set. Advertisement is three lists, and
placement is a request against them — nothing is ever pushed, only asked
for, so a browser that never advertises `shell_exec` cannot be made to run
one no matter how the request is phrased. Fuzz it three hundred ways; the
audit log holds only what was advertised.

Two markers came due and both dissolved honestly. PKG-5's worker placement
had been choosing its worker from a bool table the test handed it; now the
requester queries advertised capabilities over the wire and the first
advertiser runs the derivation, result returning by hash. PKG-6's net link
had been an in-process deque pretending to be a network; now the provider
ships its flavored delivery log across the authenticated channel and the
consumer replays it — value coalesced, events reliable-ordered, the outage
a real gap in transmission. The dissolution gate would have failed the
suite if either marker had stayed pointing at a criterion that just went
green. Neither did.

The signature that binds a capture to its author is the same ed25519 key
that is the peer's name — testimony's peer-id IS a public key, so tampering
the id and checking the signature are the same operation failing. And the
wire has a golden transcript now: the ciphertext can't be reproduced
(ephemeral keys, random nonces), but the plaintext message sequence —
varint kind, canonical dag-cbor body — is byte-identical run to run, which
is the honest thing to pin.

The crypto suite it all rides on (ADR-035) is a draft; Travis ratifies. It
is the boring choice on purpose — libsodium primitives, nothing hand-rolled,
reachability not equality for compatibility, secrecy left as decoder
scarcity for later. Peer-level conformance is born; the candidate-as-peer
harness of rung 12 now has a peer to be.
