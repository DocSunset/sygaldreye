#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=nix_shell_guard.sh
source "$(dirname "$0")/nix_shell_guard.sh"

PACKAGE="com.eyeballs.vr"
TAG="eyeballs"

bash "$(dirname "$0")/package.sh"

adb install -r build/apk/eyeballs.apk

adb logcat -c
# Use monkey to bypass the HorizonOS controller-required launch interceptor
adb shell monkey -p "$PACKAGE" -c android.intent.category.LAUNCHER 1

adb logcat -s "$TAG:V" "*:S"
