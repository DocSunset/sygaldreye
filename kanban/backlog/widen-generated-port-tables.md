# Widen generated port tables beyond hello_natives

Engine passes (engine_passes.cpp), the query four (query_natives.cpp) and
arbiter_inlet hand-write their `native_type` port tables and positional
aggregate inits, escaping AUT-3's no-hand-tables grep (scoped to
src/nodes/hello_natives). Widen generation (endpoints structs → generated
tables + named-field init) and widen the grep. Found by the rung-7
fresh-context audit, 2026-07-05.
