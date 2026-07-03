# Trust — identity, advertisement, escalation, scale

Status: ratified 2026-07-02 (architecture session, Travis + Claude). Replaces the
"visibility" section of datasets.md. Context: capability-driven placement makes
graphs remote code execution by design, so mesh membership is a security boundary.
Today's unauthenticated LAN HTTP surface is retired by this design.

## Identity: per-peer keypairs, not a shared PSK

Each peer holds a keypair; joining the mesh is a one-time pairing ceremony
(Tailscale/Bluetooth shape); the mesh is "the set of keys my peers accept." Same
flat one-domain topology a PSK would give — nobody special, everything visible
inside — but with device revocation (lose the Quest without re-keying the world),
unforgeable capture testimony (the peer id in a capture header is a public key),
and signable provenance (a shipped artifact carries who derived it). Transport
authentication/encryption falls out of peers having keys.

## Advertisement is the permission system

A peer only instantiates node types it *advertises* — placement is **pull-shaped**:
nothing can be pushed onto a peer, only requested of it. A peer is sovereign over
three published lists: what it will **run** (advertised node types), what it will
**serve** (provided datasets — see datasets.md), what it **subscribes** to.
Selective advertisement is the sandbox (edge_executor.design.md already ratified
this: a browser peer simply doesn't advertise shell-shaped nodes). Namespaces
organize; only vocabulary and refs are ever gated — never the data model.

## The one real escalation: graphs vs plugins

Two different speech acts, even inside intimate trust:

- A **graph** arriving over the mesh is composition of node types the receiving
  peer already agreed to run — bounded. Graphs flow freely in-mesh.
- A **native plugin** arriving (the LLM codegen loop: write C++, cross-compile,
  ship, live-load) is new capability injection — unbounded. Gated by **provenance
  policy**: a plugin is a dataset with provenance, so "I load plugins whose
  provenance chains to my own source datasets / to a signer I trust" is a policy a
  peer can hold with zero new machinery.

This seam is where any future multi-user story lives; keep it crisp now.

## The scale test (web replacement / package manager)

The mechanisms above are exactly the two historical answers to trust:

- **Browser sandbox** = advertisement, generalized: a stranger's graph composes
  only from the vocabulary you advertise to strangers (render/audio/input; no fs,
  no shell). The sandbox becomes declarative, per-peer, per-relationship — readable
  as a list of node names. Rhizome's "every entity carries a *proposed* program to
  render it" is safe for the same reason: the proposal is a graph, runs in your
  sandbox vocabulary, and a compatible peer may decline and render the kind its
  own way.
- **Package manager** = provenance: content-addressed derivations, signed by a
  builder you trust, or re-derive yourself and compare hashes. provide/compatible
  is the substituter relationship; a published graph with its provenance closure is
  a package; view-source is provenance-deep.

What generalizes at scale is only membership: "one flat domain" becomes the
innermost of **graded circles** — my devices (plugins flow), collaborators (graphs
+ shared store-graphs), the world (sandboxed graphs + content by hash). Gate refs,
fetch, and advertisement; never the data model. Transport is scale-dependent
(mDNS + query fan-out in the household; DHT-style discovery reinstated at world
scale); the semantics are not.
