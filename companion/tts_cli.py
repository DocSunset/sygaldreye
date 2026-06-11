#!/usr/bin/env python3
"""tts_cli.py <text-file> <out-wav> — synthesize speech with MeloTTS.

Invoked by the tts node (worker region). Run with companion/.venv-melotts.
Model load ~1.2 s per call; a warm persistent process is a known follow-up
(kanban: tts_warm_process).
"""
import sys

from melo.api import TTS

text = open(sys.argv[1]).read().strip()
out = sys.argv[2]
if not text:
    sys.exit(1)
tts = TTS(language="EN", device="cpu")
tts.tts_to_file(text, tts.hps.data.spk2id["EN-US"], out, speed=1.0)
print(out)
