# Chapter 8 — Mesh and trust (MSH)

*A builder implementing this chapter delivers: peer identity (keypairs +
pairing), authenticated transport, the three sovereign lists, the
graphs-vs-plugins gate, and the scale posture.*

## Design

**Identity.** Each peer holds a keypair; joining is a one-time pairing
ceremony; the mesh is "the set of keys my peers accept." Flat one-domain
topology for now — nobody special, everything visible inside — with device
revocation, unforgeable capture testimony (testimony's peer id IS a public
key), and signable provenance. Transport authentication/encryption falls out;
the historical unauthenticated LAN HTTP surface is retired.

**Advertisement is the permission system.** A peer instantiates only node
types it advertises — placement is pull-shaped; nothing can be pushed onto a
peer, only requested of it. A peer is sovereign over three published lists:
**run** (advertised node types), **serve** (provided datasets), **subscribe**
(live addresses it follows). Selective advertisement is the sandbox: a
browser peer simply doesn't advertise shell-shaped nodes. Namespaces
organize; only vocabulary and refs are ever gated — never the data model.

**The escalation.** A *graph* arriving in-mesh is bounded composition of what
the receiver advertised: flows freely. A *native plugin* (the LLM codegen
loop: write C++ → cross-compile → ship → live-load) is new capability
injection: a distinct speech act, gated by **provenance policy** ("load
plugins whose provenance chains to sources/signers I trust") — plugins are
datasets with signed recipes, so the gate needs no new machinery.

**Scale posture.** The mechanisms are the Web's and the package manager's
trust models already: advertisement = a declarative, per-relationship
browser sandbox; provenance = signed, reproducible packages
(re-derive-and-compare beats trust). What generalizes is only membership:
one flat domain becomes the innermost of graded circles (devices /
collaborators / world). Transport is scale-dependent (mDNS + query fan-out in
the household; DHT-style discovery at world scale); semantics are not.
Deferred deliberately: per-dataset ACLs, encryption at rest, CRDTs,
multi-writer refs (single-writer-per-ref discipline until real
collaborators). Secrecy, when it comes, is decoder scarcity (keys are
decoders) — a kind, not a mechanism.

## Requirements

**MSH-1 (keypairs + pairing).** Peers generate keypairs; pairing exchanges
and records acceptance; revocation removes a key and severs its sessions.
- MSH-1.1: three-peer pairing; revoke the Quest's key on host; Quest can no
  longer fetch, place, or subscribe; re-pairing restores.

**MSH-2 (authenticated transport).** All inter-peer traffic (control, store
fetch, net mappings) is authenticated and encrypted under the pairing keys;
no unauthenticated listener remains.
- MSH-2.1: port scan + protocol probe from an unpaired host on the LAN: every
  surface refuses; the legacy HTTP behavior is demonstrably gone.

**MSH-3 (three lists).** Advertisement of run/serve/subscribe is explicit,
queryable, and enforced at request time.
- MSH-3.1: a request to instantiate `shell_exec` on the browser peer (which
  doesn't advertise it) is refused with a typed error, and the refusal is
  visible to the requesting engine graph (compilation falls through to
  another peer or reports unplaceable).

**MSH-4 (pull-shaped placement).** There is no code path that instantiates a
node type on a peer absent from that peer's run list.
- MSH-4.1: fuzz the placement API with arbitrary type names; zero
  instantiations occur outside the advertised set (assert via registry
  audit log).

**MSH-5 (graphs vs plugins).** Graph datasets flow and realize in-mesh
without prompts; plugin datasets require the provenance-policy gate before
dlopen/side-module load.
- MSH-5.1: ship a graph Quest→host: runs. Ship an unsigned .so: refused,
  logged. Sign it with a paired key whose policy is accepted: loads
  hot (existing dlopen hot-reload machinery), and its provenance (source
  dataset, toolchain) is queryable.
- MSH-5.2: the browser peer's plugin form is a WASM side module over the
  same channel and gate; the policy check is form-agnostic.

**MSH-6 (capture testimony keys).** Every capture's testimony carries the
capturing peer's public key; verification is signature-checking, not trust.
- MSH-6.1: tamper with a take's testimony peer-id; verification fails.

**MSH-7 (discovery).** mDNS presence + direct holdings/capability queries
serve the household scale; the discovery interface is abstract enough that a
DHT provider could replace it without touching semantics.
- MSH-7.1: all MSH/STO/PKG integration tests pass with discovery swapped for
  a static peer list (proving the seam).

**MSH-8 (graded circles, seam only).** Sharing is configurable per store
graph and per advertisement list; the flat domain is expressed as "everything
shared with every paired key," not hardcoded.
- MSH-8.1: configure a second store graph shared with a subset of keys; the
  excluded peer's fetch is refused for those hashes but succeeds for the
  common store.

## Worked example (test seed)

The codegen loop under law: an agent peer authors a new node type's C++
(a capture — human/agent testimony), a worker derivation cross-compiles it
(recipe provenance), the artifact ships by hash, the Quest's policy verifies
the chain (MSH-5.1, MSH-6.1) and hot-loads it, and hello-cosine gains a new
node type mid-performance — no restart (the guiding star, exercised across
the trust boundary).
