"""Reference address parse/print — the oracle for fmt.address_grammar / nam.addresses.

Grammar per architecture/14-formats-protocols.md (ADR-029). Percent-escape
%, /, :, # and whitespace inside a local name; nothing else. An address is
(first_step_kind, first_step, [route steps...]). Executable specification —
do not optimize, do not import into the implementation.
"""
ESCAPE = set("%/:# \t\n")


def _esc(step: str) -> str:
    return "".join(f"%{ord(c):02X}" if c in ESCAPE else c for c in step)


def _unesc(s: str) -> str:
    out, i = [], 0
    while i < len(s):
        if s[i] == "%":
            out.append(chr(int(s[i + 1:i + 3], 16))); i += 3
        else:
            out.append(s[i]); i += 1
    return "".join(out)


def parse(text: str):
    if text.startswith("#"):                       # peer-key spelling
        head, *rest = text[1:].split("/")
        return ("peerkey", _unesc(head), [_unesc(r) for r in rest if r != ""])
    if ":" in text.split("/", 1)[0]:               # lexical refname spelling
        head, route = text.split(":", 1)
        steps = [_unesc(r) for r in route.split("/")] if route else []
        return ("refname", _unesc(head), steps)
    head, *rest = text.split("/")                  # cid spelling
    kind = "cid" if head.startswith("b") and len(head) > 8 else "refname"
    return (kind, _unesc(head), [_unesc(r) for r in rest if r != ""])


def print_(addr) -> str:
    kind, head, steps = addr
    route = "/".join(_esc(s) for s in steps)
    if kind == "peerkey":
        return "#" + _esc(head) + ("/" + route if route else "")
    if kind == "refname" and steps:
        return _esc(head) + ":" + route
    if kind == "refname":
        return _esc(head)
    return _esc(head) + ("/" + route if route else "")
