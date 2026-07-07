# Canonicalize port names (display vs field name footgun)

Ports have two names: the slider template string ("sun elevation", spaces)
used by edges/inlets/schema, and the PFR field name (sun_elevation) used by
params serialize/deserialize. set_*_in now accepts both
(eyeballs_node_abi detail::port_matches), but params still only accept field
names, and /values & schemas emit template names. Pick ONE canonical name
(suggest: field name), rename slider template strings to match, drop the
dual matcher. Breaks saved graphs using spaced names — migrate assets.
