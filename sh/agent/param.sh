#!/usr/bin/env bash
# Usage: param.sh <node_id> '<params-json>'   e.g. param.sh camera '{"yaw":0.5}'
set -euo pipefail
PORT="${SYGALDREYE_PORT:-8930}"
curl -sf -X POST "localhost:$PORT/param" \
    -d "{\"node\":\"${1:?node id}\",\"params\":${2:?params json}}"; echo
