# Phase 3: Speech-to-Text

## Goal

Hold a button, speak a command, release. The headset sends the audio to the Linux companion,
which transcribes it with Whisper and optionally routes it through Claude to generate a
structured graph command. No recompile, no typing, no taking off the headset.

## Deliverables

### Headset side (C++)

- `mic_capture` component: AAudio input stream (`AAUDIO_DIRECTION_INPUT`), mirrors the
  structure of the existing `audio_output` component
- Push-to-talk: hold a controller button → ring buffer accumulates PCM float samples →
  release → POST WAV (or raw PCM) to companion's `/transcribe` endpoint
- `RECORD_AUDIO` permission added to manifest

### Linux companion (Python, additions to Phase 2)

- `/transcribe` endpoint: receives audio, runs `faster-whisper` (local), returns text
- Text is parsed as a command:
  - Param update: "set wave height to eight" → POST `/params`
  - Graph command: "add a sky dome" → POST `/graph` (Phase 5+)
  - Ambiguous / natural language → Claude API → structured JSON → appropriate POST
- `ANTHROPIC_API_KEY` in companion environment; Claude call is optional/fallback

## Audio format

PCM float32, mono, headset sample rate (typically 16kHz or 48kHz). WAV header for
simplicity; the companion resamples to 16kHz if needed for Whisper.

## Whisper model selection

| Model  | Size  | RTF on modern x86 | Notes                       |
|--------|-------|-------------------|-----------------------------|
| tiny   | 39 MB | ~0.1x             | Fast, lower accuracy        |
| base   | 74 MB | ~0.2x             | Good balance for commands   |
| small  | 244 MB| ~0.5x             | Best for natural language   |

`base` is the recommended default. `faster-whisper` (CTranslate2 backend) is ~4x faster
than the original for the same model size.

## Command grammar (Phase 3 scope)

Phase 3 only needs to handle param updates — the graph doesn't exist yet. Simple heuristic
parsing suffices: "set [param name] to [value]". Full natural-language graph commands
come in Phase 5 when `/graph` exists.

## Key Design Decisions

**Push-to-talk over always-on VAD**: VAD (voice activity detection) on-device adds
complexity and battery drain. A dedicated controller button is simpler, more reliable
in noisy environments, and avoids accidental triggers. Can revisit later.

**Transcription on companion, not on-device**: The Quest 3 Snapdragon XR2 Gen 2 could
run `whisper.cpp` with the tiny/base model, but keeping ML on the Linux side means faster
iteration on the transcription pipeline without touching the C++ codebase.

**Claude API is optional**: Companion works without an API key using heuristic parsing.
Claude adds natural language understanding for graph commands when it's available.

## Dependencies

Phase 2 (networking) — companion is already running and connected.
