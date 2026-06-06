# Phase 2: Networking

## Goal

The headset and Linux companion discover each other on the LAN with zero configuration.
The headset serves a live param/graph API over HTTP. The companion mirrors param state
and serves as the gateway for all host-side processing (transcription, LLM, compilation).

## Deliverables

### Headset side (C++)

- `mdns_advertiser` component: advertises `_eyeballs._tcp.local.` on port 8080 using
  `mdns.h` (mjansson/mdns, single-header, MIT)
- `mdns_discovery` component: discovers `_eyeballs-companion._tcp.local.` and stores
  the companion's IP/port
- Android multicast lock: JNI call to `WifiManager.createMulticastLock()` +
  `lock.acquire()` at app startup (~10 lines); required for receiving multicast on Android
- HTTP server embedded in app (Mongoose, single-file C library): serves `/params` GET+POST
- Required Android permissions: `INTERNET`, `ACCESS_WIFI_STATE`,
  `CHANGE_WIFI_MULTICAST_STATE`

### Linux companion (Python)

- `companion.py`: discovers `_eyeballs._tcp.local.` via `python-zeroconf`
- Advertises `_eyeballs-companion._tcp.local.`
- Polls or subscribes to `/params` on headset; prints live param state
- Accepts POST from headset for outbound requests (audio transcription, etc.)

## HTTP API (Phase 2 subset)

```
GET  /params          → JSON object: { "water_surface.wave_height": 5.0, ... }
POST /params          → body: same JSON, partial update accepted
```

Full API surface grows in later phases (`/graph`, `/plugins`, `/meta-graph`).

## mDNS on Android: the multicast lock

Android's WiFi driver silently drops incoming multicast packets unless the app holds a
`WifiManager.MulticastLock`. Sending works without it; receiving (needed for discovery)
does not. The lock must be acquired before any mDNS socket is opened and released on
app pause/stop. This is a JNI call from the NativeActivity, placed in `app.cpp`.

## Key Design Decisions

**`mdns.h` over Avahi**: Avahi is a system daemon — not available on Android, not reliably
present on all Linux hosts. `mdns.h` is compiled in and works identically on both sides.

**Mongoose over a bespoke HTTP server**: Mongoose is a single `.c` file (embedded / no-TLS
build), handles concurrent connections, and is already used in embedded/mobile contexts.
Alternative: `httplib.h` (C++ header-only). Either is fine; pick one and don't revisit.

**Headset is the authoritative state server.** All clients POST changes to the headset;
the headset is the single source of truth. Conflict resolution is last-write-wins.

## Dependencies

Phase 1 (param registry) — the HTTP server serializes the registry to JSON.
