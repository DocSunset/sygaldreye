#!/usr/bin/env bash
# Fetch speech models into assets/models (gitignored; ~250 MB total).
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p assets/models
cd assets/models

W=https://huggingface.co/ggerganov/whisper.cpp/resolve/main
[ -f ggml-tiny.en.bin ] || curl -sL -o ggml-tiny.en.bin "$W/ggml-tiny.en.bin"
[ -f ggml-base.en.bin ] || curl -sL -o ggml-base.en.bin "$W/ggml-base.en.bin"

# Male voice for tts_local (sherpa-onnx packaged piper voice).
V=vits-piper-en_US-ryan-medium
if [ ! -d "$V" ]; then
    curl -sL -o "$V.tar.bz2" \
      "https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/$V.tar.bz2"
    tar xjf "$V.tar.bz2" && rm "$V.tar.bz2"
fi
ls -sh .
