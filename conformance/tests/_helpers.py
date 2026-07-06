"""Shared test helpers. Import as: from _helpers import *"""
import json, pathlib, random, shutil, subprocess, sys

HERE = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
FIX = HERE / "fixtures"


class Pending(Exception):
    """Raised when a test cannot run yet (e.g., no implementation binary).
    The runner counts it as pending, not failing."""


def syg(*args, stdin: bytes = b"") -> bytes:
    """Run the implementation's harness binary (HARNESS.md contract).
    Looked up as ./syg at repo root or on PATH; Pending if absent."""
    exe = HERE.parent / "syg"
    if not exe.exists():
        found = shutil.which("syg")
        if not found:
            raise Pending("implementation binary ./syg not found (HARNESS.md)")
        exe = found
    r = subprocess.run([str(exe), *args], input=stdin, capture_output=True)
    if r.returncode == 2 and b"unknown subcommand" in r.stderr:
        raise Pending(f"syg {args[0]} not implemented yet (HARNESS.md)")
    if r.returncode != 0:
        raise AssertionError(f"syg {' '.join(args)} failed: {r.stderr[:400]!r}")
    return r.stdout


def random_value(depth=0, rng=random):
    """Random dag-cbor-encodable value for differential testing."""
    kinds = ["null", "bool", "int", "float", "str", "bytes"]
    if depth < 3:
        kinds += ["list", "map", "map"]
    k = rng.choice(kinds)
    if k == "null": return None
    if k == "bool": return rng.random() < 0.5
    if k == "int": return rng.randint(-2**53, 2**53)
    if k == "float": return rng.uniform(-1e6, 1e6)
    if k == "str": return "".join(rng.choice("abč /:#%xyz") for _ in range(rng.randint(0, 12)))
    if k == "bytes": return bytes(rng.randrange(256) for _ in range(rng.randint(0, 8)))
    if k == "list": return [random_value(depth + 1, rng) for _ in range(rng.randint(0, 4))]
    return {f"k{rng.randint(0,99)}{'x'*rng.randint(0,3)}": random_value(depth + 1, rng)
            for _ in range(rng.randint(0, 4))}


def to_projection(v):
    """Python value -> the JSON projection syg speaks (HARNESS.md, DAG-JSON):
    bytes become {"/": {"bytes": <base64, no padding>}}. The oracle keeps
    seeing raw Python values; only the implementation surface is projected."""
    import base64
    if isinstance(v, bytes):
        return {"/": {"bytes": base64.b64encode(v).decode().rstrip("=")}}
    if isinstance(v, list):
        return [to_projection(x) for x in v]
    if isinstance(v, dict):
        return {k: to_projection(x) for k, x in v.items()}
    return v


def fft(x):
    """Iterative radix-2 FFT over a list of complex (len = power of 2)."""
    import math
    n = len(x)
    x = x[:]
    j = 0
    for i in range(1, n):
        bit = n >> 1
        while j & bit:
            j ^= bit
            bit >>= 1
        j |= bit
        if i < j:
            x[i], x[j] = x[j], x[i]
    length = 2
    while length <= n:
        ang = -2 * math.pi / length
        wl = complex(math.cos(ang), math.sin(ang))
        half = length // 2
        for i in range(0, n, length):
            w = 1 + 0j
            for k in range(i, i + half):
                u, v = x[k], x[k + half] * w
                x[k], x[k + half] = u + v, u - v
                w *= wl
        length <<= 1
    return x


def golden_audio_check(x, sr=48000):
    """fixtures/golden-audio.md properties 1-3 over float samples."""
    import math
    assert not any(math.isnan(v) or math.isinf(v) for v in x), "NaN/Inf in buffer"
    assert all(-1.0 <= v <= 1.0 for v in x), "sample outside [-1, 1]"
    n = 1 << 17
    assert len(x) >= n, "need >= 2.73 s at 48 kHz for the spectral check"
    w = [0.5 - 0.5 * math.cos(2 * math.pi * i / (n - 1)) for i in range(n)]
    spec = fft([complex(x[i] * w[i], 0.0) for i in range(n)])
    mag = [abs(spec[i]) for i in range(n // 2)]
    res = sr / n
    peak = max(range(1, n // 2), key=lambda i: mag[i])
    peak_f, peak_db = peak * res, 20 * math.log10(abs(mag[peak]) + 1e-30)
    assert abs(peak_f - 220.0) <= 1.0, f"dominant peak at {peak_f:.2f} Hz"
    def harmonic(f):  # the peak's harmonics carry the signal's own energy
        return any(abs(f - h * 220.0) <= 2.0 for h in range(1, int(sr / 440) + 1))
    worst = max((m for i, m in enumerate(mag) if i * res >= 5.0
                 and not harmonic(i * res)), default=0.0)
    floor_db = 20 * math.log10(worst + 1e-30)
    assert peak_db - floor_db >= 40, \
        f"non-harmonic floor only {peak_db - floor_db:.1f} dB down"
    hop = sr // 100  # 10 ms RMS envelope
    env = [math.sqrt(sum(v * v for v in x[i:i + hop]) / hop)
           for i in range(0, len(x) - hop, hop)]
    mid = (max(env) + min(env)) / 2
    rises = [i for i in range(1, len(env)) if env[i - 1] < mid <= env[i]]
    # debounce: a stepped (frame-latched) envelope can graze the midline
    # twice at one rising edge; crossings within 0.5 s are one edge
    rises = [r for i, r in enumerate(rises) if i == 0 or r - rises[i - 1] > 50]
    periods = [(b - a) / 100 for a, b in zip(rises, rises[1:])]
    assert periods, "no envelope period observed"
    assert all(abs(p - 2.0) <= 0.1 for p in periods), \
        f"envelope not 0.5 Hz +-5%: {periods}"
    depth = (max(env) - min(env)) / (max(env) + min(env))
    assert depth > 0.9, f"modulation depth {depth:.2f} below the lfo/vca defaults"


def fixture(name):
    p = FIX / name
    return p.read_text() if not name.endswith(".json") else json.loads(p.read_text())


def frame_stats(rgba, w, h, clear=(0, 0, 0, 255)):
    """fixtures/golden-frame.md pixel properties over one raw RGBA8 frame:
    foreground coverage (pixels != clear), coverage centroid in pixel
    coords, and the set of distinct colors. Properties, never bytes."""
    assert len(rgba) == w * h * 4, f"frame size {len(rgba)} != {w}x{h}x4"
    cov, sx, sy = 0, 0.0, 0.0
    colors = set()
    for i in range(w * h):
        px = tuple(rgba[4 * i:4 * i + 4])
        colors.add(px)
        if px != tuple(clear):
            cov += 1
            sx += i % w
            sy += i // w
    cx = sx / cov if cov else -1.0
    cy = sy / cov if cov else -1.0
    return {"coverage": cov, "cx": cx, "cy": cy, "colors": colors}


def ndc_to_px(x, y, w, h):
    """NDC (-1..1, y up) -> pixel coords (y down) for analytic expectations."""
    return (x + 1.0) / 2.0 * w, (1.0 - y) / 2.0 * h
