# log

Owning package: `platform`

Unified logging interface across Android (logcat) and host (spdlog/stderr).

## Ports

- Inputs: log level, tag string, printf-style format string + arguments
- Outputs: log record to logcat on Android; to spdlog (stderr) on host
- Sources: none
- Destinations: none
- Temporal couplings: none
- Intended seams: host sink can be reconfigured via spdlog API before first log call

## Requirements

- Provide `LOG_D`, `LOG_I`, `LOG_W`, `LOG_E(tag, fmt, ...)` macros
- On Android: delegate to `__android_log_print`
- On host: format via `vsnprintf`, route to spdlog (level-filtered stderr)
- Header-only; no translation unit

## Allowed dependencies

- Android system `log` library (Android only)
- spdlog (host only)
