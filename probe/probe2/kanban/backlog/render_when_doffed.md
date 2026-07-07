# App sometimes renders only when the headset is OFF (Travis, 2026-06-12)

Occasional state where the scene renders while the headset sits on the
desk but goes black/passthrough when worn — backwards from the normal
proximity-sensor behavior. Smells like xr_session state handling
(FOCUSED/VISIBLE/SYNCHRONIZED transitions, or session-state-gated
rendering inverted under some event order). Unreproduced on demand;
watch logcat session-state transitions next time it happens.

- Recurred 2026-06-12 night session (Travis poked blind through it).
  Workaround discovered: headset sleep -> wake fixes it. Strengthens the
  session-state theory (a sleep/wake cycle replays the state machine).
  Recurring + has a workaround, but should be near the top of the next
  debug session: it killed half the value of the live playground.
