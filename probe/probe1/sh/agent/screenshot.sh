#!/usr/bin/env bash
# Usage: screenshot.sh [out.png] [port] — prints the PNG path on success
set -euo pipefail
OUT="${1:-/tmp/sygaldreye_shot.png}"
PORT="${2:-${SYGALDREYE_PORT:-8930}}"
RES=$(curl -sf -X POST "localhost:$PORT/screenshot" -d "{\"path\":\"$OUT\"}")
echo "$RES" | grep -q '"ok":true' && echo "$OUT" || { echo "$RES" >&2; exit 1; }
