#!/usr/bin/env bash
# Usage: controller.sh right x y z [trigger] [grip]
#        controller.sh left  x y z [trigger] [thumb_x thumb_y]
set -euo pipefail
PORT="${SYGALDREYE_PORT:-8930}"
HAND="${1:?left|right}"; X="${2:?x}"; Y="${3:?y}"; Z="${4:?z}"
BODY="{\"hand\":\"$HAND\",\"x\":$X,\"y\":$Y,\"z\":$Z"
[[ -n "${5:-}" ]] && BODY+=",\"trigger\":$5"
if [[ "$HAND" == right ]]; then [[ -n "${6:-}" ]] && BODY+=",\"grip\":$6"
else [[ -n "${6:-}" ]] && BODY+=",\"thumb_x\":$6,\"thumb_y\":${7:-0}"; fi
curl -sf -X POST "localhost:$PORT/controller" -d "$BODY}"; echo
