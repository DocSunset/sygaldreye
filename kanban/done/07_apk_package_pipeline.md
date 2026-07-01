# 07 — APK packaging pipeline

Assemble a signed APK from the native lib (no Gradle/Java).

## Scope
- `sh/` packaging step: `aapt2 link` (manifest + android.jar), stage
  `libeyeballs.so` + `libopenxr_loader.so` into `lib/arm64-v8a/`, `zipalign`,
  `apksigner` with the debug keystore.

## Depends on
- 04 (libeyeballs.so), 02 (loader .so), 05 (manifest), 06 (keystore)

## Acceptance
- Produces a signed, zipaligned APK; `apksigner verify` passes.
