#!/usr/bin/env bash
# Launch a headless host app instance for agent use.
# Usage: launch.sh [port]   (default 8930; logs to /tmp/sygaldreye_<port>.log)
set -euo pipefail
PORT="${1:-${SYGALDREYE_PORT:-8930}}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/build/host/app/spectator/spectator"
[[ -x "$BIN" ]] || { echo "build first: cmake --build build/host --target spectator" >&2; exit 1; }
LOG="/tmp/sygaldreye_${PORT}.log"
LD_LIBRARY_PATH=/usr/lib "$BIN" --headless --port "$PORT" >"$LOG" 2>&1 &
echo $! > "/tmp/sygaldreye_${PORT}.pid"
for _ in $(seq 50); do
    curl -sf "localhost:$PORT/graph" >/dev/null 2>&1 && { echo "ready on :$PORT (log: $LOG)"; exit 0; }
    sleep 0.1
done
echo "did not become ready; see $LOG" >&2; exit 1
