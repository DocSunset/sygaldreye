# graphs/*.json are unkeyed compile inputs

Lift/subgraph definitions load from the graphs/ dir at expansion time but
never join the compile recipe's input hashes — editing audio_rules.json
would NOT bust the structural memo. They should be committed datasets whose
CIDs join the recipe (rung 9+, when vocabulary ships as data). Rung-7
audit, 2026-07-05. Also: CMP-9.4's walk-vs-registry equality is true by
construction until types load from committed datasets — same dissolution.
