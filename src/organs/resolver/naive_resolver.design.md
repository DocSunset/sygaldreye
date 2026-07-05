# resolver — the naive resolver organ

Owning package: `organs`. Clause: **machinery**. Allowed dependencies:
`formats/cid` and the filesystem — deliberately NOTHING else (SZ-3: no
dependency on any package; independently invokable when stage 1 is wedged;
the debugger of last resort; stage 0 carries only this).
hash → bytes from an on-disk object directory (one file per cid text),
verified by re-hash before returning. A corrupt object is a loud error,
never bytes.
