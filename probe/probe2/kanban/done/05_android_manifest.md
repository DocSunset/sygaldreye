# 05 — AndroidManifest.xml

Author the manifest for a pure-native Quest VR app.

## Scope
- `NativeActivity`, `android:hasCode="false"`, `meta-data android.app.lib_name=eyeballs`.
- VR launch intent: `category com.oculus.intent.category.VR` + LAUNCHER.
- `uses-feature android.hardware.vr.headtracking` (required).
- min/target SDK matching the build; landscape, no resize.

## Depends on
- nothing (can be authored anytime; consumed by 07)

## Acceptance
- Manifest validates under `aapt2 link` (verified in item 07).
