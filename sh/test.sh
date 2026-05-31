#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=nix_shell_guard.sh
source "$(dirname "$0")/nix_shell_guard.sh"

BUILD_DIR="build/host"

cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ --log-level=ERROR
cmake --build "$BUILD_DIR"

ctest --test-dir "$BUILD_DIR" --output-on-failure
