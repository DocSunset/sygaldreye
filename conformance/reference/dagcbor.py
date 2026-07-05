"""Reference canonical dag-cbor encoder — the oracle for FMT-1.

Deliberately slow and obviously correct. This file is executable
specification: an implementation is right when its bytes match this, byte
for byte, on every value. Do not optimize it; do not import it into the
implementation. Rules per architecture/14-formats-protocols.md (ADR-017).
"""
import math, struct


class Link:
    """A CID link (tag 42). value = raw CID bytes."""
    def __init__(self, cid_bytes: bytes):
        self.cid = cid_bytes


def _head(major: int, arg: int) -> bytes:
    if arg < 24:
        return bytes([major << 5 | arg])
    for ai, fmt, lim in ((24, ">B", 1 << 8), (25, ">H", 1 << 16),
                         (26, ">I", 1 << 32), (27, ">Q", 1 << 64)):
        if arg < lim:
            return bytes([major << 5 | ai]) + struct.pack(fmt, arg)
    raise ValueError("integer too large")


def encode(v) -> bytes:
    if v is None:
        return b"\xf6"
    if v is True:
        return b"\xf5"
    if v is False:
        return b"\xf4"
    if isinstance(v, int):
        return _head(0, v) if v >= 0 else _head(1, -1 - v)
    if isinstance(v, float):
        if math.isnan(v) or math.isinf(v):
            raise ValueError("NaN/Inf forbidden in dag-cbor")
        return b"\xfb" + struct.pack(">d", v)  # always 8-byte, canonical
    if isinstance(v, bytes):
        return _head(2, len(v)) + v
    if isinstance(v, str):
        u = v.encode("utf-8")
        return _head(3, len(u)) + u
    if isinstance(v, list):
        return _head(4, len(v)) + b"".join(encode(x) for x in v)
    if isinstance(v, dict):
        items = []
        for k in v:
            if not isinstance(k, str):
                raise ValueError("map keys must be strings")
            items.append((k.encode("utf-8"), encode(v[k])))
        items.sort(key=lambda kv: (len(kv[0]), kv[0]))  # length, then bytewise
        if len(set(k for k, _ in items)) != len(items):
            raise ValueError("duplicate map keys")
        return _head(5, len(items)) + b"".join(
            _head(3, len(k)) + k + val for k, val in items)
    if isinstance(v, Link):
        # tag 42 over a byte string with multibase identity prefix 0x00
        return _head(6, 42) + encode(b"\x00" + v.cid)
    raise ValueError(f"unencodable: {type(v)}")
