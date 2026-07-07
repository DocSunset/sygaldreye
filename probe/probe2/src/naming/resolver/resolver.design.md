# resolver — resolution is traversal

Owning package: `naming`. Allowed dependencies: `environment`,
`formats/address`.
Spec: ch. 2 (NAM-1/2/3) — each step is answered by the node being stepped
through, per its kind's traversal vocabulary: a graph answers `nodes/`
(through its topology, with port steps answered by the type declaration via
the lock — the worked example's walk), `topology`/`defaults`/`lock` members;
a plain node answers its keys; links fetch on demand. Fixity: live iff the
walk crosses a ref; fixed addresses normalize to a cid-rooted route.

- Inputs: a parsed address; the environment to walk from.
- Outputs: `{live, normalized, value}`; I/O observable on the environment.
- Intended seams: kind-directed traversal grows per kind (sequence kinds,
  NAM-7); the naive resolver organ (rung 4) wraps this as a node.
