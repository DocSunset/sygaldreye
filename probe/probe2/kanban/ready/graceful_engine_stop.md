# Graceful audio stream stop before force-stop / quit

Filed 2026-06-13 (was a loose todo during the distortion saga). Force-
stopping the app while AAudio streams are open leaves zombie HAL streams
looping their last buffer — only a device power-cycle clears them
(observed repeatedly; `adb reboot` is permission-gated, so the human had
to power-cycle the headset by hand to recover clean audio).

## Fix

On app teardown / quit, stop and close every open audio stream BEFORE
the process dies:
- Hook the device path (audio_output_android / the engine's stream
  ownership) into the quit/destroy sequence so streams drain + close
  cleanly.
- Cover both exits: XR session quit (should_quit loop) and external
  `am force-stop` (register an atexit / signal handler if force-stop
  gives us a teardown window — verify it does; if not, document that
  force-stop always needs the graceful /quit route).

## Acceptance

- Quit and relaunch on device with an active synth chain produces clean
  audio immediately, no power-cycle, repeatably.
- Document the safe shutdown path in audio_output's design doc.

Related: audio_region_followups.md (device synths still own private
AAudio streams — migrating them to the shared dac shrinks the surface).
