#!/usr/bin/env python3
"""melotts_smoke.py — synthesize a phrase with MeloTTS to a WAV, time it.

Run inside the .venv-melotts environment. First run downloads the English
checkpoint + bert-base-uncased (~650 MB) into the HF cache.
"""
import sys
import time

from melo.api import TTS

TEXT = sys.argv[1] if len(sys.argv) > 1 else (
    "The quick brown fox jumps over the lazy dog. "
    "MeloTTS is running locally on this machine, CPU only."
)
OUT = "/tmp/melotts_smoke.wav"

t0 = time.time()
tts = TTS(language="EN", device="cpu")
load_s = time.time() - t0
speaker_id = tts.hps.data.spk2id["EN-US"]

t1 = time.time()
tts.tts_to_file(TEXT, speaker_id, OUT, speed=1.0)
synth_s = time.time() - t1

print(f"model load:  {load_s:6.2f}s")
print(f"synthesis:   {synth_s:6.2f}s  ({len(TEXT)} chars)")
print(f"wrote {OUT}")
