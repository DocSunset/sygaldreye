#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=nix_shell_guard.sh
source "$(dirname "$0")/nix_shell_guard.sh"

FILES=$(find . -name '*.cpp' -o -name '*.hpp' | grep -v build/)

if [[ "${1:-}" == "--check" ]]; then
    clang-format --dry-run --Werror $FILES
else
    clang-format -i $FILES
fi
