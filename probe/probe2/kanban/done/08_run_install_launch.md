# 08 — install, launch, logcat

Wire `sh/run.sh` to deploy and run on the Quest.

## Scope
- `sh/run.sh`: build → package → `adb install -r` → `am start` the activity →
  stream filtered `adb logcat`.

## Depends on
- 07

## Acceptance
- **Milestone:** blank `NativeActivity` installs and launches on the Quest 3;
  lifecycle logs visible.
