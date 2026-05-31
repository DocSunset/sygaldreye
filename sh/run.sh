#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=nix_shell_guard.sh
source "$(dirname "$0")/nix_shell_guard.sh"

# Pushes the Android binary to a connected device/emulator via adb and runs it.
: "${ANDROID_NDK_ROOT:?ANDROID_NDK_ROOT must be set (enter nix devShell first)}"

BINARY="build/android/app/hello/app_hello"

if [[ ! -f "$BINARY" ]]; then
    echo "Binary not found. Run sh/build.sh first." >&2
    exit 1
fi

adb push "$BINARY" /data/local/tmp/app_hello
adb shell chmod +x /data/local/tmp/app_hello
adb shell /data/local/tmp/app_hello
