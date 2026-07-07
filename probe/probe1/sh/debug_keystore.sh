#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=nix_shell_guard.sh
source "$(dirname "$0")/nix_shell_guard.sh"

KEYSTORE_PATH="build/debug.keystore"

if [[ -f "$KEYSTORE_PATH" ]]; then
    echo "Keystore already exists at $KEYSTORE_PATH"
    exit 0
fi

mkdir -p build
keytool -genkeypair \
    -keystore "$KEYSTORE_PATH" \
    -alias androiddebugkey \
    -storepass android \
    -keypass android \
    -keyalg RSA \
    -keysize 2048 \
    -validity 10000 \
    -dname "CN=Android Debug,O=Android,C=US"

echo "Debug keystore generated at $KEYSTORE_PATH"
