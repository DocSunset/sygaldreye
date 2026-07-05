"""Reference CID assembly and text form — oracle half of NAM-6 (the hash
itself is pinned by fixtures/blake3-vectors.json; Python has no blake3).
CIDv1 per architecture/14-formats-protocols.md: 0x01, varint multicodec,
multihash blake3-256 (0x1e, 0x20), 32-byte digest; text is multibase 'b' +
RFC 4648 base32, lowercase, no padding. Executable specification — do not
optimize, do not import into the implementation."""
import base64

DAG_CBOR, RAW = 0x71, 0x55


def _varint(n: int) -> bytes:
    out = bytearray()
    while n >= 0x80:
        out.append(n & 0x7F | 0x80)
        n >>= 7
    out.append(n)
    return bytes(out)


def cid_bytes(multicodec: int, digest: bytes) -> bytes:
    assert len(digest) == 32
    return b"\x01" + _varint(multicodec) + _varint(0x1E) + _varint(32) + digest


def text(cid: bytes) -> str:
    return "b" + base64.b32encode(cid).decode().lower().rstrip("=")
