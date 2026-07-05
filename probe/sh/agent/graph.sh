#!/usr/bin/env bash
# Usage: graph.sh get | graph.sh post file.json | graph.sh palette  [--port N]
set -euo pipefail
PORT="${SYGALDREYE_PORT:-8930}"
CMD="${1:?get|post|palette}"; shift || true
[[ "${1:-}" == --port ]] && { PORT="$2"; set --; }
case "$CMD" in
    get)     curl -sf "localhost:$PORT/graph" ;;
    palette) curl -sf "localhost:$PORT/palette" ;;
    post)    curl -sf -X POST "localhost:$PORT/graph" --data-binary "@${1:?json file}" ;;
esac; echo
