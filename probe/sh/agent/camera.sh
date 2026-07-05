#!/usr/bin/env bash
# Usage: camera.sh [x y z [yaw pitch]] [--port N] — no args: print camera
set -euo pipefail
PORT="${SYGALDREYE_PORT:-8930}"
args=()
while (($#)); do [[ "$1" == --port ]] && { PORT="$2"; shift 2; } || { args+=("$1"); shift; }; done
if ((${#args[@]} == 0)); then curl -sf "localhost:$PORT/camera"; echo; exit; fi
BODY="{\"x\":${args[0]},\"y\":${args[1]},\"z\":${args[2]}"
((${#args[@]} >= 5)) && BODY+=",\"yaw\":${args[3]},\"pitch\":${args[4]}"
curl -sf -X POST "localhost:$PORT/camera" -d "$BODY}"; echo
