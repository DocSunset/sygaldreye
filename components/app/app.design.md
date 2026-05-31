# app

Android NativeActivity entry point. Runs `android_main`, logs a startup message, and returns.

## Ports

**Sources**
- `struct android_app* app` — OS lifecycle and event state provided by `native_app_glue`; behavior depends on the Android runtime.

**Intended seams**
- `android_app*` parameter — the glue struct; replaceable for future extension.

## Requirements

- Implement `android_main(struct android_app*)`.
- Log a startup message via Android log.
- Return immediately (no event loop yet).

## Allowed dependencies

*(none — NDK only)*

## Owning package

platform
