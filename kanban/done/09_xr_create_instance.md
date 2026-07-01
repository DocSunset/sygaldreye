# 09 — xrCreateInstance

Create the OpenXR instance.

## Component
`platform` / `app`

## Scope
- Init the loader for Android (`xrInitializeLoaderKHR` with JNI/context).
- `xrCreateInstance` enabling `XR_KHR_android_create_instance` +
  `XR_KHR_opengl_es_enable`; populate `XrInstanceCreateInfoAndroidKHR`.
- Log runtime name + version. Expose `XrInstance` as an output port.

## Depends on
- 04

## Acceptance
- logcat shows the Meta runtime name on launch.
