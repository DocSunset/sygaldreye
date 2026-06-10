#!/usr/bin/env bash
# Usage: values.sh [--port N] — dump all node.port values from last tick
set -euo pipefail
PORT="${SYGALDREYE_PORT:-8930}"
[[ "${1:-}" == --port ]] && PORT="$2"
curl -sf "localhost:$PORT/values"; echo
