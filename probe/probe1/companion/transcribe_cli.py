#!/usr/bin/env python3
"""transcribe_cli.py <wav> — print the transcript of a WAV file.

Invoked by the whisper node (worker region). Run with companion/.venv.
"""
import sys

from faster_whisper import WhisperModel

model = WhisperModel("base.en", device="cpu", compute_type="int8")
segments, _info = model.transcribe(sys.argv[1], beam_size=5)
print(" ".join(s.text.strip() for s in segments).strip())
