#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=nix_shell_guard.sh
source "$(dirname "$0")/nix_shell_guard.sh"

: "${ANDROID_NDK_ROOT:?ANDROID_NDK_ROOT must be set (enter nix devShell first)}"

TARGET="${1:-water_surface_snapshot}"
EXTRA_ARGS=("${@:2}")

BUILD_DIR="build/android"
TOOLCHAIN="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake"

if ! adb get-state 2>/dev/null | grep -q '^device$'; then
    echo "No headset connected via adb" >&2
    exit 1
fi

cmake -S . -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-26 \
    -DANDROID_STL=c++_static \
    -DCMAKE_BUILD_TYPE=Debug \
    --log-level=ERROR

cmake --build "$BUILD_DIR" --target "$TARGET"

DEVICE_DIR=/data/local/tmp/eyeballs_snapshots
REMOTE_OUT="$DEVICE_DIR/${TARGET}.png"
LOCAL_OUT="${SNAPSHOT_OUT_DIR:-/tmp}/${TARGET}.png"

BIN="$(find "$BUILD_DIR/components" -name "$TARGET" -type f | head -1)"
if [[ -z "$BIN" ]]; then
    echo "Binary '$TARGET' not found under $BUILD_DIR/components" >&2
    exit 1
fi

adb shell mkdir -p "$DEVICE_DIR"
adb push "$BIN" "$DEVICE_DIR/$TARGET"
adb shell chmod +x "$DEVICE_DIR/$TARGET"
adb shell "$DEVICE_DIR/$TARGET" "$REMOTE_OUT" ${EXTRA_ARGS[@]+"${EXTRA_ARGS[@]}"}
adb pull "$REMOTE_OUT" "$LOCAL_OUT"
echo "Saved: $LOCAL_OUT"
