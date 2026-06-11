#!/usr/bin/env bash
# Claude Code Stop hook: speak the reply via the host graph's tts node
# (MeloTTS on the worker region → POSTs the wav to the headset's /play).
# Falls back to the legacy companion /tts if the graph peer is down.
set -uo pipefail
payload=$(cat)
text=$(echo "$payload" | jq -r '.last_assistant_message // empty')
[[ -n "$text" ]] || exit 0
text=$(echo "$text" | sed '/```/,/```/d' | head -c 1200)
seq=$(date +%s%N)
body=$(jq -n --arg t "$text" --arg s "$seq" \
       '{node:"speak",params:{message:$t,seq:($s|tonumber)}}')
curl -sf -X POST "${GRAPH_PARAM_URL:-http://127.0.0.1:8930/param}" \
     --data-binary "$body" >/dev/null && exit 0
curl -sf -X POST "${COMPANION_TTS_URL:-http://127.0.0.1:9090/tts}" \
     -H 'Content-Type: application/json' \
     -d "$(jq -n --arg t "$text" '{text:$t}')" >/dev/null || true
