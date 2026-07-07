# 04 — app lifecycle event loop

Service the `native_app_glue` command/event loop cleanly.

## Component
`platform` / `app`

## Scope
- Poll `ALooper` / `android_app` sources; dispatch `APP_CMD_*`.
- Handle init/term window, gained/lost focus, `APP_CMD_DESTROY` (request exit).
- Track a `resumed && window` "renderable" flag as an output port.

## Depends on
- 03

## Acceptance
- App stub runs its loop and exits cleanly on destroy (logged); still builds.
