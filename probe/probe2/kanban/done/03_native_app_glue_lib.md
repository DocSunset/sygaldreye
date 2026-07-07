# 03 — native_app_glue + app stub

Bootstrap the `platform`/`app` component as a shared lib.

## Component
`platform` / `app`

## Scope
- cmake static lib for NDK `native_app_glue` (from `$ANDROID_NDK_ROOT/sources/android/native_app_glue`).
- `components/app/` with `android_main(android_app*)` that logs and returns.
- Build as `libeyeballs.so` (the APK's `android.app.lib_name`).
- Start `app.design.md` (ports: OS events in via `android_app`).

## Depends on
- 02

## Acceptance
- `libeyeballs.so` builds for arm64; `android_main` symbol present.
