# Recorded WAVs contain interleaved garbage floats (NaN / ~FLT_MAX)

Quest session 2026-06-11: recordings transcribe fine (whisper is robust)
but ~45% of early samples are garbage. Stream reports FLOAT format
honestly (I16-reinterpretation ruled out; conversion guard added anyway).
Suspect a lifetime/race on the emitted AudioBuffer borrowed pointer —
exactly the stream-edge lifetime contract the edge/executor design names.
Likely resolves with ring mappings (build step 5); investigate then.
Repro: record via stt node, autopsy companion /tmp/tmp*.wav.
