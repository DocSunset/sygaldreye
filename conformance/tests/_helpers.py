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


def fixture(name):
    p = FIX / name
    return p.read_text() if not name.endswith(".json") else json.loads(p.read_text())
