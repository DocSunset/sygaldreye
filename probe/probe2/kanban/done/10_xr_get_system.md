# 10 — xrGetSystem

Acquire the HMD system.

## Component
`platform` / `app`

## Scope
- `xrGetSystem` for `FORM_FACTOR_HEAD_MOUNTED_DISPLAY`.
- Query + log `XrSystemProperties`. Expose `XrSystemId` as an output port.

## Depends on
- 09

## Acceptance
- logcat shows system id + properties.
