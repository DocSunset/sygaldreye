# mic_input teardown heap corruption (device crash 2026-06-12)

Swapping a graph that removes the mic node killed the app ~15 s later:
SIGSEGV inside jemalloc on a VrDriver thread = classic heap corruption,
surfaced far from the cause. Strong suspect: mic_input's input-stream
callback kept writing after its buffers were freed by the swap —
the same bug class as the fixed block-scheduler swap race, but
pause_blocks() only guards the OUTPUT side; input streams have no
equivalent.

Fix: mic_input destructor must stop AND join/drain its stream before
buffers go away. Better (Travis's hint, and the JACK model): ONE duplex
audio stream per process — mic samples delivered by the same callback
that renders output, so there is exactly one audio thread whose
lifecycle pause_blocks() already controls. Fold into the JACK backend
work.

Likely the same animal as mic_garbage_samples.md (callback reading/
writing buffers with the wrong lifetime) and possibly yesterday's
"weird state" reports. Also re-test full-duplex FPS after: mic+dac
duplex was an early framerate suspect (acquitted — wires were the
cause — but never measured clean).
