#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=nix_shell_guard.sh
source "$(dirname "$0")/nix_shell_guard.sh"

: "${ANDROID_NDK_ROOT:?ANDROID_NDK_ROOT must be set (enter nix devShell first)}"

BUILD_DIR="build/android"
TOOLCHAIN="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake"

cmake -S . -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-26 \
    -DANDROID_STL=c++_static \
    -DCMAKE_BUILD_TYPE=Debug \
    --log-level=ERROR

cmake --build "$BUILD_DIR" --target hello_test

DEVICE_DIR=/data/local/tmp/eyeballs_tests
adb shell mkdir -p "$DEVICE_DIR"
adb push "$BUILD_DIR/components/hello/hello_test" "$DEVICE_DIR/hello_test"
adb shell chmod +x "$DEVICE_DIR/hello_test"
adb shell "$DEVICE_DIR/hello_test"
