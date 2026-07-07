# ugen decomposition follow-ups

- hrtf_params subgraph form of spatialize (mapper node emitting
  coefficients into generic delay/shelf/notch ugens) — purist refactor,
  monolithic spatialize node works today.
- Legacy retirement AFTER device A/B (headset session): delete the seven
  *_synth node classes + audio_scene + wav_player; swap quest voice
  graph's speaker → sample_player; synth_core stays as the ugen impl lib.
- Block scalar outputs only reach /values via a crossing edge (by
  design); consider a probe mapping for block nodes so agents can read
  e.g. speaker.playing without wiring a dummy consumer.
- Preset sound design pass with Travis live (all params are sliders now).
- 50-ugen perf patch + consolidation study redo (phase 4).
- JACK backend for AudioOutput (~80 lines, same callback contract):
  pipewire-jack on modern linux = pro scheduling + sygaldreye as a
  patchable JACK client (DAW routing). Also the glitch diagnosis: ear
  test showed occasional spontaneous glitches under SDL2→PipeWire @256
  samples; if they persist under JACK, suspect my scheduler. → overnight.
- POST /plugins JSON naming is key-order-fragile (top-level "name" must
  precede "nodes"; cost us two silent no-op reloads in the headphone
  session). Fix: scan top-level keys properly or take ?name= query.
  Bit again 2026-06-11, twice: (1) the substring match finds NESTED
  "name" keys, so a preset whose JSON starts with its inlets array
  registered as type "freq"; (2) the needle is exactly `"name":"` —
  pretty-printed JSON (space after colon) misses, falls back to
  preset_<epoch>, and uploads in the same second overwrite each other.
  Asset files now lead with a compact top-level "name" as a workaround;
  the handler needs a real JSON parse.
- Headphone session proved inlet_defaults rung 1 by absence: chime decay
  was unreachable live without a whole-type hot reload (no decay inlet,
  subgraph params dropped). Rung 1 = overnight 2.
- SWAP/AUDIO RACE (FIXED 2026-06-11, found by Travis's first 30s of
  in-headset poking): graph swap destroyed old node instances while the
  audio callback held raw pointers mid-block → device SIGSEGV in
  SubgraphNode tick. Fix: swap+migrate+old-destruction+region-rebuild
  all under audio_.pause_blocks() (peer_core + web shell). Host had the
  same latent bug. Test idea: stress-loop graph swaps against a running
  offline pump with ASAN.
- spawner drops nodes at type defaults → spawned cubes hid inside the
  scenery cube ("nothing happened"). Spawner wants spawn-position params
  (or spawn-at-tip from the poke rig).
- POST /plugins hot-reload races POST /graph: each plugin upload queues
  a re-serialize of the RUNNING graph; a /graph posted in between can be
  clobbered when that queued edit drains. Saw it as a vanished test
  graph (2026-06-12 overnight). Order edits, or tag re-parse edits so a
  newer explicit /graph wins.
