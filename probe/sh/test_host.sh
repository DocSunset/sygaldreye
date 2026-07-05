#!/usr/bin/env bash
# Build and run every host gtest binary. Exit nonzero on any failure.
set -euo pipefail
cd "$(dirname "$0")/.."
nix develop --command bash -c \
    'cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Debug --log-level=ERROR > /dev/null && cmake --build build/host' > /dev/null
fail=0
for t in $(find build/host -name '*_test' -type f); do
    if out=$(LD_LIBRARY_PATH=/usr/lib "$t" 2>&1 | tail -1); then
        echo "PASS  $(basename "$t")  $out"
    else
        echo "FAIL  $(basename "$t")"; fail=1
    fi
done
exit $fail
