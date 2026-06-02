#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=nix_shell_guard.sh
source "$(dirname "$0")/nix_shell_guard.sh"

PACKAGE="com.eyeballs.vr"
TAG="eyeballs"

bash "$(dirname "$0")/package.sh"

install_out=$(adb install -r build/apk/eyeballs.apk 2>&1) || {
    if echo "$install_out" | grep -q "INSTALL_FAILED_UPDATE_INCOMPATIBLE"; then
        echo "Signature mismatch — uninstalling and retrying"
        adb uninstall "$PACKAGE"
        adb install build/apk/eyeballs.apk
    else
        echo "$install_out" >&2
        exit 1
    fi
}

adb logcat -c
# Use monkey to bypass the HorizonOS controller-required launch interceptor
adb shell monkey -p "$PACKAGE" -c android.intent.category.LAUNCHER 1

adb logcat -s "$TAG:V" "*:S"
