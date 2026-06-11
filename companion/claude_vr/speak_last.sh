#!/usr/bin/env bash
# Claude Code Stop hook: speak the reply via companion /tts.
set -uo pipefail
payload=$(cat)
text=$(echo "$payload" | jq -r '.last_assistant_message // empty')
[[ -n "$text" ]] || exit 0
text=$(echo "$text" | sed '/```/,/```/d' | head -c 1200)
curl -sf -X POST "${COMPANION_TTS_URL:-http://127.0.0.1:9090/tts}" \
     -H 'Content-Type: application/json' \
     -d "$(jq -n --arg t "$text" '{text:$t}')" >/dev/null || true
