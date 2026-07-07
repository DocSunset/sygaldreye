#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=nix_shell_guard.sh
source "$(dirname "$0")/nix_shell_guard.sh"

: "${ANDROID_SDK_ROOT:?ANDROID_SDK_ROOT must be set (enter nix devShell first)}"

BUILD_TOOLS="$ANDROID_SDK_ROOT/build-tools/34.0.0"
ANDROID_JAR="$ANDROID_SDK_ROOT/platforms/android-34/android.jar"
export PATH="$BUILD_TOOLS:$PATH"

# Build native libs
bash "$(dirname "$0")/build.sh"

EYEBALLS_SO=$(find build/android -name 'libeyeballs.so' | head -1)
LOADER_SO=$(find build/android -name 'libopenxr_loader.so' | head -1)
: "${EYEBALLS_SO:?libeyeballs.so not found}"
: "${LOADER_SO:?libopenxr_loader.so not found}"

# Debug keystore
bash "$(dirname "$0")/debug_keystore.sh"

# Staging
APK_DIR=build/apk
LIB_DIR="$APK_DIR/lib/arm64-v8a"
rm -rf "$APK_DIR"
mkdir -p "$LIB_DIR"
cp "$EYEBALLS_SO" "$LOADER_SO" "$LIB_DIR/"

# Link manifest into base APK (no resources to compile)
aapt2 link \
    -o "$APK_DIR/eyeballs-unsigned.apk" \
    -I "$ANDROID_JAR" \
    --manifest app/AndroidManifest.xml

# Add native libs (stored uncompressed for extractNativeLibs=true)
(cd "$APK_DIR" && zip -0 eyeballs-unsigned.apk lib/arm64-v8a/libeyeballs.so lib/arm64-v8a/libopenxr_loader.so)

# Zipalign
zipalign -f -p 4 "$APK_DIR/eyeballs-unsigned.apk" "$APK_DIR/eyeballs-aligned.apk"

# Sign
apksigner sign \
    --ks build/debug.keystore \
    --ks-pass pass:android \
    --key-pass pass:android \
    --out "$APK_DIR/eyeballs.apk" \
    "$APK_DIR/eyeballs-aligned.apk"

# Verify
apksigner verify "$APK_DIR/eyeballs.apk"
echo "APK verified."

unzip -l "$APK_DIR/eyeballs.apk" | grep 'lib/'
