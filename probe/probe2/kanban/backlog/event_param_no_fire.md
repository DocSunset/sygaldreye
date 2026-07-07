# bang via /param doesn't fire block-region event_in (perc.trigger)

2026-06-12, device: POST /param {"node":"ding1","params":{"trigger":1}}
never raised ding1.level, while {"gate":1} on the same node works (so
the param write path reaches the instance). v6 set_scalar_in writes
event_in.triggered=true; PercNode reads it but something prevents the
block tick from seeing it (or it is consumed/cleared first). In-graph
event paths (poke crossing queues) are unaffected. Also note PercNode
never resets triggered — if a param write DID land it would retrigger
every block; semantics need deciding (read-once self-reset like
spawner/tts, probably).
