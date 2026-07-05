"""Rung 4 — liveness organs. Tests written from the criterion text as they
are reached (BUILDER.md loop). Never weaken a test; amend the book by ADR."""
import json, pathlib
from _helpers import Pending, syg, HERE

ROOT = HERE.parent


def lng81_roundtrip():
    corpus = sorted((ROOT / "graphs").glob("*.json"))
    assert corpus, "graphs_dir corpus missing"
    for f in corpus:
        j = json.loads(f.read_text())
        assert "mappings" not in json.dumps(j), f"derived structure persisted in {f}"
        back = json.loads(syg("roundtrip", stdin=f.read_bytes()))
        assert back == j, f"round-trip not identity for {f}"
        # idempotence: serialize of the parsed form is a fixpoint
        assert json.loads(syg("roundtrip", stdin=json.dumps(back).encode())) == back


def _tape(name):
    return (ROOT / "conformance" / "fixtures" / name).read_text()


def _swap_render(tape1, tape2, seconds):
    import tempfile
    with tempfile.NamedTemporaryFile("w", suffix=".tape") as f:
        f.write(tape2)
        f.flush()
        return syg("swap-audit", f.name, str(seconds), stdin=tape1.encode())


def exe51_state_survives_swap():
    # while sounding, swap hello-cosine for a version with an added noise0:
    # osc0's phase (and every migrated state) is continuous — the swapped
    # render is byte-identical to an uninterrupted one
    v1 = _tape("hello-cosine.tape")
    v2 = v1 + "NODE noise0 noise\n"
    straight = syg("render-tape", "2", stdin=v1.encode())
    swapped = _swap_render(v1, v2, 2)
    assert swapped == straight, "state did not survive the swap"


def exe52_clones_keyed_survive_reorder():
    # keyed clones (lift_key rides the route: osc#<key>): reordering the
    # span leaves each clone's state with its key, not its position
    nodes_v1 = "NODE osc#a osc\nNODE osc#b osc\nNODE osc#c osc\n"
    nodes_v2 = "NODE osc#c osc\nNODE osc#b osc\nNODE osc#a osc\n"  # reorder
    rest = ("NODE mix0 add\nNODE mix1 add\nNODE dac0 dac\n"
            "LINK osc#a/out mix0/a\nLINK osc#b/out mix0/b\n"
            "LINK mix0/out mix1/a\nLINK osc#c/out mix1/b\n"
            "LINK mix1/out dac0/in\n"
            "SET osc#a/freq 220.0\nSET osc#b/freq 330.0\nSET osc#c/freq 440.0\n")
    straight = syg("render-tape", "2", stdin=(nodes_v1 + rest).encode())
    swapped = _swap_render(nodes_v1 + rest, nodes_v2 + rest, 2)
    assert swapped == straight, \
        "clone state followed position, not key, across the reorder"


def sz31_naive_resolver():
    import tempfile
    # the store package does not exist here (stage 1 "wedged") — the naive
    # resolver needs only an object directory and a hash
    graph = json.dumps(json.loads(
        (ROOT / "graphs" / "hello-cosine.json").read_text())).encode()
    canonical = syg("encode", stdin=graph)
    cid = syg("hash", stdin=canonical).decode().strip()
    with tempfile.TemporaryDirectory() as td:
        (pathlib.Path(td) / cid).write_bytes(canonical)
        assert syg("resolve-hash", cid, td) == canonical
        # a corrupted object never leaves the resolver
        bad = bytearray(canonical)
        bad[len(bad) // 2] ^= 1
        (pathlib.Path(td) / cid).write_bytes(bytes(bad))
        try:
            syg("resolve-hash", cid, td)
            raise AssertionError("corrupt object was served")
        except AssertionError as e:
            assert "verification failed" in str(e), e
        try:
            syg("resolve-hash", "b" + "a" * 20, td)
            raise AssertionError("miss did not error")
        except AssertionError as e:
            assert "object miss" in str(e), e


def sz21_registration_by_linkage():
    import shutil, subprocess, tempfile
    manifest = json.loads((ROOT / "build" / "generated" / "registration.json")
                          .read_text())
    # the palette equals the manifest plus graphs_dir datasets (LNG-6):
    # every native in the manifest is present, and every extra palette
    # entry is a graph dataset on disk — nothing appears from nowhere
    palette = json.loads(syg("palette"))
    assert [p for p in palette if p in manifest] == manifest
    for extra in (set(palette) - set(manifest)):
        assert (ROOT / "graphs" / f"{extra}.json").exists(), extra
    gxx = shutil.which("g++")
    if not gxx:
        raise Pending("no C++ compiler for the linkage audit")
    # the generated TU REQUIRES each native's symbol: nm -u of its object
    # names every native, so a deleted object is a loud link error
    with tempfile.TemporaryDirectory() as td:
        obj = pathlib.Path(td) / "registration.o"
        nix_json = subprocess.run(
            ["pkg-config", "--cflags", "nlohmann_json"], capture_output=True,
            text=True).stdout.split()
        subprocess.run([gxx, "-std=c++20", *nix_json,
                        "-I", str(ROOT / "src" / "crown"),
                        "-I", str(ROOT / "src" / "escapement"),
                        "-I", str(ROOT / "src" / "organs"),
                        "-c", str(ROOT / "build" / "generated" / "registration.cpp"),
                        "-o", str(obj)], check=True)
        undefined = subprocess.run(["nm", "-uC", str(obj)], capture_output=True,
                                   text=True, check=True).stdout
        for native in manifest:
            symbol = native.replace("-", "_") + "_native"
            assert symbol in undefined, \
                f"registration does not require {native}'s object at link time"
        # and actually linking without the objects fails, naming the symbol
        main = pathlib.Path(td) / "main.cpp"
        main.write_text('#include "registry_face/registry_face.hpp"\n'
                        "int main() { return (int)syg::organs::registered_natives().size(); }\n")
        r = subprocess.run([gxx, "-std=c++20", *nix_json,
                            "-I", str(ROOT / "src" / "crown"),
                            "-I", str(ROOT / "src" / "escapement"),
                            "-I", str(ROOT / "src" / "organs"),
                            str(main), str(obj)], capture_output=True, text=True)
        assert r.returncode != 0 and "osc_native" in r.stderr, \
            "omitting a native's object did not break the link by name"


def sz41_unfreeze_stage0():
    out = json.loads(syg("unfreeze-stage0"))
    # rebuild compares: the embedded tape is the source file, byte for byte,
    # and both hashes re-derive from the repo
    src = (ROOT / "src" / "stage0" / "boot.tape").read_text()
    assert out["tape"] == src, "embedded tape diverged from its source"
    assert out["tape_cid"] == syg("hash", stdin=src.encode()).decode().strip()
    reg = json.loads((ROOT / "build" / "generated" / "registration.json")
                     .read_text())
    assert out["manifest"] == reg, \
        "stage-0 manifest is not the linked registry"
    # re-derive the manifest cid: canonical-encode, hash, re-assemble under
    # the dag-cbor multicodec (digest extracted via the cid oracle)
    import base64
    from reference import cid as cidref
    raw_cid = syg("hash", stdin=syg("encode", stdin=json.dumps(
        out["manifest"]).encode())).decode().strip()
    digest = base64.b32decode(raw_cid[1:].upper() + "=" * (-len(raw_cid[1:]) % 8))[-32:]
    assert out["manifest_cid"] == cidref.text(
        cidref.cid_bytes(cidref.DAG_CBOR, digest)), "manifest cid mismatch"


def sz51_spawn_and_park():
    out = json.loads(syg("park-audit"))
    assert out["kills"] == 100 and out["restarts"] == 100, out
    # SZ-5.2: edits addressed at stage 0 are refused with a clear error
    assert "stage 0 rejects runtime edits" in out["stage0_edit_refused"], out


def sz7_boot_without_store():
    import tempfile
    with tempfile.TemporaryDirectory() as td:  # an EMPTY object directory
        out = json.loads(syg("boot-audit", td))
        assert not any(pathlib.Path(td).iterdir()), "boot touched the store"
    ids = {n.split(":")[0] for n in out["nodes"]}
    assert {"parser0", "resolver0", "registry0", "engine0", "super0"} <= ids, \
        f"liveness organs not instated: {out['nodes']}"
    assert out["post_boot_op_landed"] is True, "the booted peer is not live"


def sz8_the_ladder():
    import shutil, struct, subprocess
    from _helpers import golden_audio_check
    # (a) the boot path is op application only: every node the peer holds
    # was built by replaying the embedded tape's records
    import tempfile
    with tempfile.TemporaryDirectory() as td:
        out = json.loads(syg("boot-audit", td))
    tape_records = [ln for ln in (ROOT / "src" / "stage0" / "boot.tape")
                    .read_text().splitlines()
                    if ln.strip() and not ln.startswith("#")]
    assert out["boot_ops"] == len(tape_records)
    assert len(out["nodes"]) == sum(1 for r in tape_records if r.startswith("NODE"))
    # (b) a crownless movement build passes movement-level conformance only:
    # the sealed binary renders the golden take and links NO crown at all
    firmware = ROOT / "build" / "hello-cosine-movement"
    if not firmware.exists():
        raise Pending("sealed movement binary not built yet")
    raw = subprocess.run([str(firmware), "6"], capture_output=True,
                         check=True).stdout
    golden_audio_check(struct.unpack(f"<{len(raw) // 4}f", raw))
    if shutil.which("nm"):
        syms = subprocess.run(["nm", "-C", str(firmware)], capture_output=True,
                              text=True).stdout
        assert "crown" not in syms, "a sealed movement must omit the crown"


TESTS = {
    "EXE-5.1": exe51_state_survives_swap,
    "EXE-5.2": exe52_clones_keyed_survive_reorder,
    "LNG-8.1": lng81_roundtrip,
    "SZ-2.1": sz21_registration_by_linkage,
    "SZ-3.1": sz31_naive_resolver,
    "SZ-4.1": sz41_unfreeze_stage0,
    "SZ-5.1": sz51_spawn_and_park,
    "SZ-7": sz7_boot_without_store,
    "SZ-8": sz8_the_ladder,
}
