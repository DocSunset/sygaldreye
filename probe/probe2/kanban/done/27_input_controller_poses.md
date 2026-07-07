# 27 — input component (controller poses)

New component for OpenXR action input. Narrow — only action set setup and per-frame controller pose location.

## Component
- Name: `input`, owning package: `xr` (per architecture).
- Allowed dependencies: OpenXR loader, platform (log).

## Ports
- Source: `XrSession`, `XrInstance` (from xr_session), world `XrSpace`.
- Input `create(session, instance)` — create action set, a grip-pose action bound to `/user/hand/left|right`, suggest bindings, attach action set, create one pose `XrSpace` per hand.
- Input `sync(XrTime)` — `xrSyncActions`, locate each hand space → `XrPosef` + valid flag.
- Output `hand_pose(hand) -> {XrPosef, bool valid}`.
- Temporal coupling: `create()` after session; `sync()` per frame after xrBeginFrame, before drawing.

## Scope
- Header + cpp. Log binding suggestion result and first valid pose.

## Depends on
- nothing (uses existing xr_session outputs)

## Acceptance
- On device, both controllers report valid poses (logged) when tracked.
