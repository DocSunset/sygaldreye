#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=nix_shell_guard.sh
source "$(dirname "$0")/nix_shell_guard.sh"

: "${ANDROID_NDK_ROOT:?ANDROID_NDK_ROOT must be set (enter nix devShell first)}"

BUILD_DIR="build/android"
TOOLCHAIN="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake"

if [[ "${1:-}" == "--clean" ]]; then
    rm -rf "$BUILD_DIR"
fi

cmake -S . -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-26 \
    -DANDROID_STL=c++_static \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build "$BUILD_DIR"
