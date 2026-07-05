#!/usr/bin/env bash
# Usage: stop.sh [port]
set -euo pipefail
PORT="${1:-${SYGALDREYE_PORT:-8930}}"
curl -sf -X POST "localhost:$PORT/quit" >/dev/null || true
rm -f "/tmp/sygaldreye_${PORT}.pid"
echo "stopped :$PORT"
