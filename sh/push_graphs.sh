#!/usr/bin/env bash
# Push assets/graphs/*.json subgraph presets to the headset's graphs dir
# (loaded at boot; individual presets can also be hot-loaded any time via
# POST /plugins). Usage: sh/push_graphs.sh [adb-serial]
set -euo pipefail
cd "$(dirname "$0")/.."
PKG=com.eyeballs.vr
SERIAL="${1:+-s $1}"

for f in assets/graphs/*.json; do
    base=$(basename "$f")
    adb $SERIAL push "$f" "/data/local/tmp/$base" >/dev/null
    adb $SERIAL shell run-as $PKG sh -c "'mkdir -p files/graphs && cp /data/local/tmp/$base files/graphs/$base'"
    echo "pushed $base"
done
