# app

Android NativeActivity entry point. Runs the `native_app_glue` event loop, dispatches lifecycle commands, and exposes a `renderable` output port for downstream components.

## Ports

**Inputs**
- `APP_CMD_INIT_WINDOW` / `APP_CMD_TERM_WINDOW` — window created/destroyed
- `APP_CMD_GAINED_FOCUS` / `APP_CMD_LOST_FOCUS` — focus change
- `APP_CMD_DESTROY` — OS requests exit

**Outputs**
- `AppState::renderable()` — `true` when `hasWindow && hasFocus`; consumers poll this to know whether to render
- `AppState::xrInstance` — OpenXR instance handle; valid after `android_main` initializes it
- `AppState::xrSystemId` — OpenXR HMD system id; valid after `xr_get_system` succeeds

**Sources**
- `struct android_app* app` — OS lifecycle and event state provided by `native_app_glue`

**Temporal couplings**
- Must call `ALooper_pollOnce` every iteration; must honour `destroyRequested` and return promptly

**Intended seams**
- `android_app*` parameter — replaceable for future extension

## Requirements

- Run event loop until `destroyRequested`, then return.
- Set poll timeout to 0 when `renderable`, -1 otherwise (no busy-spin before window exists).
- Log each handled lifecycle command.

## Allowed dependencies

- OpenXR SDK (`openxr_loader`)

## Owning package

platform
