# Device plugin .so crashes at node instantiation (2026-06-11 session)
NDK-compiled poke_stick plugin: uploads+registers OK, but spawning the
node SIGSEGVs at null fn-ptr inside parse_graph (pc 0; backtrace in
logcat ~23:16). Host plugin loop works. Also: /plugins uploads flaky on
device (intermittent "symbol not found" = suspected truncation — add
length/checksum to POST /plugins). Three fixes already found en route:
-fno-gnu-unique is glibc-only; plugins need -static-libstdc++; uploads
need retry. Needs a dedicated session: ndk-stack the tombstone, check
static-init of the descriptor under bionic + static libc++.

## RESOLVED (2026-06-12 evening)
Fresh repro attempt on the current build: poke_button compiled with
compile_node.py (--target android), uploaded via /plugins (772 KB,
clean), hot-reload re-parse OK, spawned instance ticks (hover/pressed/
render in /values), survives repeated swap cycles. No tombstone.
Likely killed by the ABI version-guards (the old null-fn-ptr crash
matches reading v6 descriptor fields off a shorter struct) and/or the
larger mongoose recv buffer (upload truncation). compile_node.py fixed
to accept EIGEN3_INCLUDE_DIR (env name drift). THE DEVICE PLUGIN LOOP
IS OPEN: code ships to the running headset.
