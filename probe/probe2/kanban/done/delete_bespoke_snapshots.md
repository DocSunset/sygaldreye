# Delete the bespoke per-node snapshot executables

scene_snapshot's per-node exes (sky_dome_snapshot, aurora_snapshot, ...)
and sh/snapshot.sh are superseded by the live app + sh/agent/screenshot.sh
and the /values probe. They no longer track node decomposition (sky lost
its embedded stars and three of them broke silently). Propose: delete the
exes + macro block + sh/snapshot.sh; keep the write_snapshot library
function while app_snapshot (Android) still uses it. Needs Travis's nod —
it removes an old workflow entry point.
