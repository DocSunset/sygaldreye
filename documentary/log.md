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
