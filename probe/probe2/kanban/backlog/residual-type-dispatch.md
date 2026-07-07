# Residual type-string dispatch outside realization

query_eval.cpp:18-22 and store_session.cpp standing-delta injection
(`t == "seed" && watches`) dispatch on type strings — data preparation, not
evaluation, but the policed pattern (ADR-034). Dissolve when watch-flags
and query seeding become graph-expressed. Rung-7 audit, 2026-07-05.
