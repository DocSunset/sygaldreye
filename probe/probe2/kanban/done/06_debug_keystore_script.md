# 06 — debug keystore

Generate a debug signing key for APK signing.

## Scope
- `sh/` helper that creates a debug keystore via `keytool` if absent (idempotent).
- Git-ignore the keystore.

## Depends on
- nothing

## Acceptance
- Running it produces a keystore; re-running is a no-op.
