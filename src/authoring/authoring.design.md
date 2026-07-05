# authoring — the endpoints surface

Owning package: `authoring`. Allowed dependencies: none (header-only tags).
Spec: ch. 12/13 (AUT-3, ABI-1), ADR-017/030 — a node type is authored as ONE
endpoints struct with real named fields; the field's type carries its port
promises (kind + discipline as tag types). Everything else — descriptor,
codec, bindings, shell — is generated from it; authors never write
serialization, registration, or try/catch.

- Intended seams: new kinds/disciplines are new tag types (packages bring
  their own); the generator reads them through `port_traits`.
