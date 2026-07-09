# Chapter 17 — Conformance and evolution (CNF)

*The suite that IS the system, and how everything changes without anything
breaking. Governing ADRs: 021, 025, 026, 027.*

## Conformance

**The suite is the definition** (ADR-026): an implementation is sygaldreye
iff it passes. Two profiles (ADR-028):

- **movement-level** (behavioral): given these graphs/tapes, produce these
  outputs; carry provenance; honor severance/defaults semantics. What a
  sealed firmware must pass.
- **peer-level** (protocol): pairing, advertisement, ops, fetch,
  subscription, placement refusal — fmt.wire_transcripts's golden transcripts plus live
  property tests. What a mesh citizen must pass.

**The suite is written in the system** (ADR-027): a test is a derivation —
test graph + blessed expected outputs + a comparison program — with
comparison semantics per determinism class (ADR-021): exact to hash
equality; approximate to tolerance graphs (spectral diff for audio,
perceptual for images). **The harness boots the candidate as a peer** and
exercises it over the wire: conformance testing and interop testing are one
act. The suite evolves by succession, provenance-tracked; any conformant
implementation judges any candidate.

**Blessing is testimony.** Golden expected outputs enter by capture — a
human (or a delegated agent) once said "this is right" — signed, dated,
supersedable. The suite's ground is honest about being fiat.

**Trusting-trust**: conformance is behavioral and judged by *other* peers;
with multiple implementations, cross-derivation is diverse
double-compilation. Structural answer, standing.

## Evolution

- **Nothing is ever maintained in place** (ADR-025): nodes are succeeded;
  declared succession (+ migration) in proportion to fan-in; migration is
  lazy, memoized, provenance-recorded derivation; structural migrations are
  exact-class so upgrades hash identically mesh-wide.
- **Vocabulary upgrades are one lock swap** (ADR-026): a graph's lock
  object maps names to type CIDs; a vocabulary migration rewrites locks,
  never topologies.
- **Mixed-version mesh**: peers advertise the kinds/contracts they speak;
  compatibility = reachability over supersedes/migrates-from (a fixpoint
  query, lng.query_vocabulary); an old peer fetches a migration (it's a dataset) or asks
  a capable peer to derive the upgrade (placement).
- **Versions are derived** (ADR-032): each declared succession carries a
  class — breaking | additive | fix; `name@M.m.p` is computed from the chain
  by a fixpoint query, stored nowhere; additive/fix classes are VERIFIED
  (predecessor's criteria stay green; old decoders still decode); breaking
  requires a migration or an explicit none. Semver as a checkable projection.
- **The core evolves too** (ADR-027): sources are datasets, the core binary
  is a derivation, sygaldreye-N derives N+1; the suite gates every
  succession of everything, including itself.

## Requirements

**cnf.suite_as_data** Every acceptance criterion in this book exists
as a test dataset (graph + blessing + comparator) queryable by chapter and
requirement id; CI = run the suite as derivations; a criterion without a
test is itself a reported gap (the suite tests its own coverage).
**cnf.candidate_as_peer** The harness runs entirely over the wire
protocol against a candidate binary it did not link — proven by running it
against (a) the reference core and (b) a deliberately broken mutant (which
must fail with the failing requirement named).
**cnf.two_profiles** The movement profile passes on a crownless build;
the peer profile fails on it (absence of the wire surface is detected, not
excused).
**cnf.succession_end_to_end** Introduce kind-v2 with a migration:
old objects readable via lazy migrate-on-read (memo hit second time);
mixed-version two-peer exchange works both directions; lock-swap upgrades
one graph without touching its topology hash's provenance chain.
**cnf.derived_versions** Succession classes are declared,
versions computed, classes verified.
- cnf.derived_versions.version_chain: a chain of successions (origin, fix, additive, breaking, fix)
  derives `@0.0.1`, `@0.1.0`, `@1.0.0`, `@1.0.1` at the respective heads.
- cnf.derived_versions.additive_verified: an "additive" succession that fails a predecessor criterion is
  rejected by the gate with the failing criterion named.
- cnf.derived_versions.version_address_resolves: `osc@1.2.0` as a ref-sugar address resolves to the same hash as
  walking the chain manually.

**cnf.self_gate** A core succession (N derives N+1) is admitted only
by the suite run by N (and, when they exist, by non-N implementations).
