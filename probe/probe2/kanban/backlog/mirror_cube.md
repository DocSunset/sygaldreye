# Mirror cube (deferred 2026-06-12)

Make the middle playground cube replay the last recorded voice take:
take_path/clip as a graph payload (span clip or path text edge) ->
sample_player, cube pressed -> play bang. Blocked v1 attempt: take was
not a first-class edge payload. Spans now exist; revisit after planar
audio (named-shape Span) makes clips portable data.
