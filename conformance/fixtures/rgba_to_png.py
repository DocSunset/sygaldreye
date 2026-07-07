#!/usr/bin/env python3
"""Encode raw top-down RGBA8 bytes to a PNG (stdlib only — no Pillow).

    syg frame < scene.json > frame.rgba 2>/dev/null
    python3 rgba_to_png.py frame.rgba WIDTH HEIGHT out.png [FRAME_INDEX]

Used to make the golden-frame blessing image (fixtures/golden-frame.md):
render the first-light triangle, encode a PNG, send it to Travis.
"""
import struct
import sys
import zlib


def write_png(rgba: bytes, w: int, h: int, path: str) -> None:
    raw = bytearray()
    for y in range(h):
        raw.append(0)  # filter type 0 (none) per scanline
        raw.extend(rgba[y * w * 4:(y + 1) * w * 4])

    def chunk(typ: bytes, data: bytes) -> bytes:
        return (struct.pack(">I", len(data)) + typ + data +
                struct.pack(">I", zlib.crc32(typ + data) & 0xFFFFFFFF))

    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)  # 8-bit RGBA
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", ihdr))
        f.write(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
        f.write(chunk(b"IEND", b""))


if __name__ == "__main__":
    src, w, h, out = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
    idx = int(sys.argv[5]) if len(sys.argv) > 5 else 0
    data = open(src, "rb").read()
    stride = w * h * 4
    write_png(data[idx * stride:(idx + 1) * stride], w, h, out)
    print(f"wrote {out} ({w}x{h}, frame {idx})")
