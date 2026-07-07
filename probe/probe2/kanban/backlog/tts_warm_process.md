# tts/whisper nodes: keep models warm

tts_node and whisper_stt spawn a fresh python per message (worker
region). MeloTTS pays ~15 s model load cold (~4 s synthesis); whisper
base.en reloads per call too. companion.py kept both warm in-process.

Fix: persistent worker processes (spawn once, feed lines over stdin,
read results over stdout) — still graph-expressed, same node interface,
command param becomes the long-running process. Do this before the next
live headset voice session or replies will feel sluggish compared to the
companion-era espeak.

- 2026-06-12 live session: MeloTTS message->speech latency judged *very*
  long by Travis in the headset voice loop; voice quality liked, but he
  prefers a MALE voice. Warm process should fix most of the latency
  (cold model load per call); pick a male speaker/model when integrating,
  else fall back to a lighter TTS.
