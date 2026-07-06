# Guides

Practitioner guides for authoring in sygaldreye. The book (`architecture/`) is
the spec and the law; these are how-to. When they disagree, the book wins.

- **[Authoring a graph](authoring-a-graph.md)** — start here. The default way to
  make anything: nodes, edges, defaults, subgraphs, the four routes, freezing.
- **[Authoring a native](authoring-a-native.md)** — the exception. When and how
  to drop to C++: the clause you must declare, declaration-first ports, the
  kernel floor, registration.
- **[Making live edits as an LLM peer](live-edits-as-an-llm-peer.md)** — you are
  a peer, not a backdoor: pairing and advertisement, the op vocabulary, the
  three ways to send an edit, projection editing, observability, and the codegen
  loop under law.

The one line under all three: **make graphs, not C++; if it can't be a graph
yet, build the leaf that lets it, then author the graph** (L22).
