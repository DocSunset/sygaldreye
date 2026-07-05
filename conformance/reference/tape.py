"""Reference boot-tape parser — the oracle for FMT-3 (ADR-028).

Record grammar (pinned, ch. 14): newline-delimited ASCII, space-separated
fields, percent-escaped as in address.py. NODE id kind | LINK from to |
SET route value. Replays to a plain dict {nodes, edges, defaults}.
"""
from .address import _unesc


def replay(text: str) -> dict:
    g = {"nodes": {}, "edges": [], "defaults": {}}
    for n, line in enumerate(text.split("\n"), 1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        op, *args = [_unesc(f) for f in line.split(" ") if f != ""]
        if op == "NODE" and len(args) == 2:
            g["nodes"][args[0]] = {"type": args[1]}
        elif op == "LINK" and len(args) == 2:
            g["edges"].append({"from": args[0], "to": args[1]})
        elif op == "SET" and len(args) == 2:
            v = args[1]
            try:
                v = float(v) if "." in v or v.lstrip("-").isdigit() else v
            except ValueError:
                pass
            g["defaults"][args[0]] = v
        else:
            raise ValueError(f"tape line {n}: bad record {line!r}")
    return g
