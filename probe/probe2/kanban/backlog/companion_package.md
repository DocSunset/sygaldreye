# Companion package — agents as peers, cognition as patches

Sketched 2026-07-04 (architecture session). NOT language design — a capability
package spending only ratified vocabulary; the core is agent-blind and nothing
in it depends on this. Supersedes the probe-grade companion/ + claude_tmux.

- **An agent is a peer**: keys, advertisement, subscriptions, attributed edit
  ops. Host: a Python process embedding the core (ADR-019) wrapping an LLM
  SDK loop; or any remote peer speaking the protocol.
- **Vocabulary**: `llm` node (worker region; inputs: context links + model id
  + sampling params; output: response dataset; declared fault output).
  Conversation/document kind (system prompt dataset; human turns = captures/
  testimony; agent turns = derivations of the preceding context).
- **An LLM call is a derivation** (ADR-021 applied): nondeterministic at
  temperature, ~approximate at temp-0+seed. Memo on context hashes = prompt
  caching for free, mesh-wide. Provenance answers "why did it say that"
  structurally; re-running from turn N is a ref fork.
- **Context assembly is a patch**: queries (ADR-024) + transclusion assemble
  the window — the agent's cognition pipeline is visible and rewirable, not
  vendor string-concatenation. Skills/memory = datasets + standing indexes.
- **Tools are the sandbox**: the agent's tool list IS what its host peer
  advertises to it (MSH-3); seeing = subscribing to probes/screenshots/
  spectrograms (EDR-8); acting = ops + source nodes. No new permissions.
- **Model placement is capability placement**: llm machinery (API client or
  local weights) lives on whichever peer advertises it.
- Multi-agent = multiple peers; ADR-023's arbiter + attribution already
  govern collaboration.
