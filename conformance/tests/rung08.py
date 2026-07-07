"""Rung 8 — packages & freezer. Tests written from the criterion text as
they are reached (BUILDER.md loop). Never weaken; amend by ADR."""
import json, re, subprocess, tempfile
from pathlib import Path
from _helpers import Pending, syg, HERE

ROOT = HERE.parent


def _hello():
    return json.loads((ROOT / "graphs" / "hello-cosine.json").read_text())


def _chime():
    return json.loads((ROOT / "graphs" / "chime.json").read_text())


def _peer(ops):
    return json.loads(syg("peer", stdin=json.dumps({"ops": ops}).encode()))["results"]


_BACKEND_SPLICE = [
    {"op": "add_node", "a": "backend0", "b": "text_cell"},
    {"op": "set_text", "a": "backend0/value", "b": "codegen"},
    {"op": "add_edge", "a": "backend0/out", "b": "realize0/backend"},
]


def _freeze(graph, extra_ops=()):
    # freeze = compile with backend codegen (ADR-014): the backend arrives
    # by WIRING a text cell into realize's published port — ordinary ops
    r = _peer([
        {"op": "set-app", "graph": graph},
        {"op": "open-engine-editor"},
        {"op": "engine-edit", "ops": _BACKEND_SPLICE + list(extra_ops)},
        {"op": "compile"},
        {"op": "app-cid"},
    ])
    return r[3], r[4]["cid"]


def _build_so(source, tag):
    d = Path(tempfile.mkdtemp(prefix=f"syg-frozen-{tag}-"))
    (d / "frozen.cpp").write_text(source)
    so = d / "libfrozen.so"
    cc = subprocess.run(
        ["g++", "-std=c++20", "-O2", "-fPIC", "-shared",
         f"-I{ROOT}/src/nodes", f"-I{ROOT}/src/crown",
         f"-I{ROOT}/src/escapement",
         "-o", str(so), str(d / "frozen.cpp")], capture_output=True)
    assert cc.returncode == 0, cc.stderr.decode()
    return so


def _stat(args, stdin=b""):
    # run syg capturing BOTH streams; stderr carries the block-time stat
    exe = ROOT / "syg"
    r = subprocess.run([str(exe), *args], input=stdin, capture_output=True)
    assert r.returncode == 0, r.stderr.decode()
    return r.stdout, json.loads(r.stderr.splitlines()[-1])["ns_per_block"]


def frz11_ab_chime_interpreted_vs_frozen():
    # A/B chime interpreted-vs-frozen: spectrogram diff == 0 (byte
    # identity, stronger); block-time stat shows the speedup; hot-reload
    # swaps it live (one process, the artifact loads mid-stream)
    c, _ = _freeze(_chime())
    x = c["execution_body"]
    assert x["backend"] == "codegen" and "artifact" in x, x.keys()
    source = _peer([{"op": "set-app", "graph": _chime()},
                    {"op": "open-engine-editor"},
                    {"op": "engine-edit", "ops": _BACKEND_SPLICE},
                    {"op": "compile"},
                    {"op": "cat", "cid": x["artifact"]["/"]}])[4]["bytes"]
    assert "frozen_movement" in source
    so = _build_so(source, "chime")
    interp, interp_ns = _stat(["render-graph", "2"],
                              stdin=json.dumps(_chime()).encode())
    frozen, frozen_ns = _stat(["render-frozen", str(so), "2"])
    import struct
    xs = struct.unpack(f"<{len(interp) // 4}f", interp)
    assert max(abs(x) for x in xs) > 0.5, "the chime never struck"
    assert interp == frozen, \
        "freezing changed the sound, not just the cost (byte diff)"
    assert frozen_ns < interp_ns,         f"no speedup: frozen {frozen_ns} ns/block vs interpreted {interp_ns}"
    # hot-reload: ONE process renders interpreted, loads the .so live at a
    # block boundary, and the artifact takes over the stream (fresh state:
    # the re-strike is the artifact's own t=0; migration joins EXE-5 later)
    swap = syg("render-swap", str(so), "2", stdin=json.dumps(_chime()).encode())
    half = len(interp) // 2
    assert swap[:half] == interp[:half], "the interpreted half diverged"
    assert swap[half:] == frozen[:half], "the artifact did not take over live"
    # the 2026-07-05 audit's regression shapes, pinned forever:
    # (a) a straight-line ≥block delay — the impulse lands at EXACTLY the
    # delay length, regardless of node names (scheduling must never
    # depend on names), byte-identical both backends
    for name in ("pulse0", "apulse0"):
        g = {"kind": "graph", "lock": {},
             "topology": {"nodes": {name: {"type": "pulse"},
                                    "delay0": {"type": "delay"},
                                    "dac0": {"type": "dac"}},
                          "edges": [{"from": f"{name}/out", "to": "delay0/in"},
                                    {"from": "delay0/out", "to": "dac0/in"}]},
             "defaults": {"delay0/samples": 200.0}}
        ii = syg("render-graph", "1", stdin=json.dumps(g).encode())
        ixs = struct.unpack(f"<{len(ii) // 4}f", ii)
        first = next(i for i, x in enumerate(ixs) if x != 0.0)
        assert first == 200, f"[{name}] impulse at {first}, wanted 200"
        c2, _ = _freeze(g)
        src2 = _peer([{"op": "set-app", "graph": g},
                      {"op": "open-engine-editor"},
                      {"op": "engine-edit", "ops": _BACKEND_SPLICE},
                      {"op": "compile"},
                      {"op": "cat",
                       "cid": c2["execution_body"]["artifact"]["/"]}])[4]["bytes"]
        ff = syg("render-frozen", str(_build_so(src2, f"cut-{name}")), "1")
        assert ii == ff, f"[{name}] cut-delay straight line diverged"
    # (b) a ≥block delay inside a REAL cycle: the cut edge carries ONE
    # BLOCK of latency (echo period = delay + block), both backends
    loop = {"kind": "graph", "lock": {},
            "topology": {"nodes": {"pulse0": {"type": "pulse"},
                                   "add0": {"type": "add"},
                                   "delay0": {"type": "delay"},
                                   "fb0": {"type": "vca"},
                                   "dac0": {"type": "dac"}},
                         "edges": [{"from": "pulse0/out", "to": "add0/a"},
                                   {"from": "fb0/out", "to": "add0/b"},
                                   {"from": "add0/out", "to": "delay0/in"},
                                   {"from": "delay0/out", "to": "fb0/in"},
                                   {"from": "add0/out", "to": "dac0/in"}]},
            "defaults": {"delay0/samples": 200.0, "fb0/gain": 0.9}}
    li = syg("render-graph", "1", stdin=json.dumps(loop).encode())
    lxs = struct.unpack(f"<{len(li) // 4}f", li)
    echoes = [i for i, x in enumerate(lxs) if x != 0.0][:3]
    assert echoes == [0, 328, 656], f"cut-cycle semantics moved: {echoes}"
    c3, _ = _freeze(loop)
    src3 = _peer([{"op": "set-app", "graph": loop},
                  {"op": "open-engine-editor"},
                  {"op": "engine-edit", "ops": _BACKEND_SPLICE},
                  {"op": "compile"},
                  {"op": "cat",
                   "cid": c3["execution_body"]["artifact"]["/"]}])[4]["bytes"]
    lf = syg("render-frozen", str(_build_so(src3, "loop")), "1")
    assert li == lf, "cut-cycle block-carry diverged between backends"
    # (c) the frame/latch emission path (lfo -> vca), never A/B'd before
    hi = syg("render-graph", "2", stdin=json.dumps(_hello()).encode())
    c4, _ = _freeze(_hello())
    src4 = _peer([{"op": "set-app", "graph": _hello()},
                  {"op": "open-engine-editor"},
                  {"op": "engine-edit", "ops": _BACKEND_SPLICE},
                  {"op": "compile"},
                  {"op": "cat",
                   "cid": c4["execution_body"]["artifact"]["/"]}])[4]["bytes"]
    hf = syg("render-frozen", str(_build_so(src4, "hello")), "2")
    assert hi == hf, "the frozen frame/latch path diverged"


def aut21_no_raw_frame_loops():
    # grep gate for raw frame loops in ugens; exceptions annotated. The
    # stamp generates the rest (leaf_natives.cpp lives in build/generated
    # and never enters this scan).
    hits, annotated = [], 0
    for f in sorted((ROOT / "src" / "nodes").rglob("*.cpp")):
        lines = f.read_text().splitlines()
        for i, ln in enumerate(lines):
            if re.search(r"for \(int i = 0; i < frames", ln):
                window = "\n".join(lines[max(0, i - 3):i + 1])
                if "AUT-2" in window:
                    annotated += 1
                else:
                    hits.append(f"{f.relative_to(ROOT)}:{i + 1}")
    assert not hits, f"unannotated raw frame loops: {hits}"
    assert annotated > 0, "the gate scanned nothing — scope is wrong"


def aut22_stamp_preserves_block_semantics():
    # the kernel-extraction acceptance, kept green forever: a GENERATED
    # stamp (leaf natives) and the hand stamp render BYTE-IDENTICAL audio
    # over the same signal (IEEE-754: x + y == x - (-y) exactly), and the
    # blessed golden still holds after any stamp refactor
    import struct
    from _helpers import golden_audio_check
    def two_osc(mid_nodes, mid_edges):
        return {"kind": "graph", "lock": {},
                "topology": {"nodes": {"osc0": {"type": "osc"},
                                       "osc1": {"type": "osc"},
                                       "dac0": {"type": "dac"},
                                       **mid_nodes},
                             "edges": [{"from": "osc0/out", "to": mid_edges[0]},
                                       {"from": "osc1/out", "to": mid_edges[1]}]
                             + mid_edges[2]},
                "defaults": {"osc0/freq": 220.0, "osc1/freq": 331.0}}
    hand = two_osc({"add0": {"type": "add"}},
                   ["add0/a", "add0/b",
                    [{"from": "add0/out", "to": "dac0/in"}]])
    leaf = two_osc({"neg0": {"type": "neg"}, "sub0": {"type": "sub"}},
                   ["sub0/a", "neg0/a",
                    [{"from": "neg0/out", "to": "sub0/b"},
                     {"from": "sub0/out", "to": "dac0/in"}]])
    a = syg("render-graph", "1", stdin=json.dumps(hand).encode())
    b = syg("render-graph", "1", stdin=json.dumps(leaf).encode())
    assert a == b, "the generated stamp diverged from the hand stamp"
    # and the ugens suite\'s golden acceptance is unchanged
    raw = syg("render-graph", "6", stdin=json.dumps(_hello()).encode())
    golden_audio_check(struct.unpack(f"<{len(raw) // 4}f", raw))


def frz12_unfreeze_is_reading_provenance():
    # unfreeze(artifact) yields the source graph hash; re-freezing is a
    # memo hit — one session, one store, ordinary ops throughout
    r = _peer([
        {"op": "set-app", "graph": _chime()},
        {"op": "open-engine-editor"},
        {"op": "engine-edit", "ops": _BACKEND_SPLICE},
        {"op": "compile"},
        {"op": "app-cid"},
    ])
    c, app_cid = r[3], r[4]["cid"]
    artifact = c["execution_body"]["artifact"]["/"]
    r2 = _peer([
        {"op": "set-app", "graph": _chime()},
        {"op": "open-engine-editor"},
        {"op": "engine-edit", "ops": _BACKEND_SPLICE},
        {"op": "compile"},
        {"op": "unfreeze", "artifact": artifact},
        {"op": "compile"},
        {"op": "app-cid"},
    ])
    assert r2[4]["app"] == r2[6]["cid"] == app_cid, \
        "unfreezing did not walk provenance back to the source graph"
    assert r2[5]["memo"] is True, "re-freezing missed the memo"


def frz21_tier_derived_from_native_closure():
    # hello-cosine's block region reports tier 1; add a tmux node and the
    # enclosing graph reports tier 3 with the culprit named
    c, _ = _freeze(_hello())
    x = c["execution_body"]
    assert x["tier"] == 1, x
    tainted = _hello()
    tainted["topology"]["nodes"]["tmux0"] = {"type": "tmux"}
    c2, _ = _freeze(tainted)
    x2 = c2["execution_body"]
    assert x2["tier"] == 3, x2
    assert "tmux0" in x2["tier_culprit"], \
        f"the culprit is unnamed: {x2['tier_culprit']!r}"


def frz31_arm_freestanding_link():
    # arm-none-eabi build of frozen chime links clean (long before real
    # hardware): the artifact's dependency closure is kernel headers only
    import shutil
    if not shutil.which("nix"):
        raise Pending("nix unavailable for the cross toolchain")
    r = _peer([
        {"op": "set-app", "graph": _chime()},
        {"op": "open-engine-editor"},
        {"op": "engine-edit", "ops": _BACKEND_SPLICE},
        {"op": "compile"},
    ])
    artifact = r[3]["execution_body"]["artifact"]["/"]
    source = _peer([
        {"op": "set-app", "graph": _chime()},
        {"op": "open-engine-editor"},
        {"op": "engine-edit", "ops": _BACKEND_SPLICE},
        {"op": "compile"},
        {"op": "cat", "cid": artifact}])[4]["bytes"]
    d = Path(tempfile.mkdtemp(prefix="syg-arm-"))
    (d / "tu.cpp").write_text(source + """
int main() {
  static frozen_movement m;
  m.init();
  static float buf[128];
  for (int b = 0; b < 8; ++b) m.pump_block(buf);
  volatile float sink = buf[0];
  (void)sink;
  return 0;
}
""")
    cc = subprocess.run(
        ["nix", "shell", "nixpkgs#gcc-arm-embedded", "-c",
         "arm-none-eabi-g++", "-O2", "-ffreestanding", "-DSYG_FREESTANDING",
         "--specs=nosys.specs", f"-I{ROOT}/src/nodes",
         "-o", str(d / "chime.elf"), str(d / "tu.cpp"), "-lm"],
        capture_output=True, timeout=600)
    if cc.returncode != 0 and b"could not be found" in cc.stderr:
        raise Pending("gcc-arm-embedded not fetchable here")
    assert cc.returncode == 0, cc.stderr.decode()
    assert (d / "chime.elf").exists()


def aut51_four_routes_one_registry():
    # hello-cosine's osc swapped for (a) a subgraph osc, (b) a frozen osc
    # artifact, (c) a shipped plugin osc — same patch JSON otherwise, same
    # golden audio. All three routes render BYTE-IDENTICAL to the native
    # (stronger than the criterion's tolerance; ledgered).
    import binascii, struct
    from _helpers import golden_audio_check

    def swapped(t):
        g = _hello()
        g["topology"]["nodes"]["osc0"] = {"type": t}
        return g

    def take(graph, pre_ops=()):
        r = _peer(list(pre_ops) + [
            {"op": "set-app", "graph": graph},
            {"op": "render-take", "blocks": 2250},
        ])
        return binascii.unhexlify(r[-1]["hex"])

    base = take(_hello())
    golden_audio_check(struct.unpack(f"<{len(base) // 4}f", base))

    # (a) the SUBGRAPH route: graphs/osc_sub.json, inlets carry the params
    sub = take(swapped("osc_sub"))
    assert sub == base, "the subgraph osc diverged from the native"

    # (b) the FROZEN route: freeze an osc movement; the artifact IS a
    # plugin (node-type contract emitted alongside the movement contract)
    osc_only = {"kind": "graph", "lock": {},
                "topology": {"nodes": {"t0": {"type": "osc"},
                                       "dac0": {"type": "dac"}},
                             "edges": [{"from": "t0/out", "to": "dac0/in"}]},
                "defaults": {"t0/freq": 220.0, "t0/shape": "cosine"}}
    c, _ = _freeze(osc_only)
    artifact = c["execution_body"]["artifact"]["/"]
    source = _peer([{"op": "set-app", "graph": osc_only},
                    {"op": "open-engine-editor"},
                    {"op": "engine-edit", "ops": _BACKEND_SPLICE},
                    {"op": "compile"},
                    {"op": "cat", "cid": artifact}])[4]["bytes"]
    frozen_so = _build_so(source, "osc")
    frozen = take(swapped("osc_frozen"),
                  pre_ops=[{"op": "load-plugin", "so": str(frozen_so),
                            "as": "osc_frozen"}])
    assert frozen == base, "the frozen osc artifact diverged from the native"

    # (c) the SHIPPED PLUGIN route: an out-of-tree .so through the gate
    plugin_so = _build_so(
        (ROOT / "conformance" / "fixtures" / "plugin_osc.cpp").read_text(),
        "plugin")
    plug = take(swapped("plugin_osc"),
                pre_ops=[{"op": "load-plugin", "so": str(plugin_so)}])
    assert plug == base, "the shipped plugin osc diverged from the native"

    # palette-identical: all routes answer from the ONE registry view
    r = _peer([{"op": "load-plugin", "so": str(plugin_so)},
               {"op": "load-plugin", "so": str(frozen_so), "as": "osc_frozen"},
               {"op": "registry-promises", "type": "plugin_osc", "port": "out"},
               {"op": "registry-promises", "type": "osc_frozen", "port": "out"}])
    assert r[2]["promises"]["kind"] == "audio"
    assert r[3]["promises"]["kind"] == "audio"


def pkg11_package_omitted_at_build_time():
    # a package can be omitted from a target at build time and the system
    # boots with that vocabulary simply absent from the palette
    import shutil, os
    if not shutil.which("nix"):
        raise Pending("nix unavailable to configure the omitted target")
    bd = ROOT / "build-omit"
    r = subprocess.run(
        ["nix", "develop", "-c", "sh", "-c",
         f"cmake -B {bd} -G Ninja -DSYG_OMIT=audio -Wno-dev > /dev/null "
         f"&& ninja -C {bd} syg"],
        cwd=ROOT, capture_output=True, timeout=900)
    assert r.returncode == 0, r.stderr.decode()[-2000:]
    exe = bd / "syg"
    pal = json.loads(subprocess.run([str(exe), "palette"],
                                    cwd=ROOT, capture_output=True).stdout)
    for t in ("osc", "dac", "delay", "mul", "tanh_"):
        assert t not in pal, f"omitted audio vocabulary leaked: {t}"
    for t in ("parser", "slot", "realize", "seed", "text_cell"):
        assert t in pal, f"non-audio vocabulary vanished: {t}"
    # the system BOOTS: stage 0 unfreezes and a peer session answers
    boot = subprocess.run([str(exe), "unfreeze-stage0"],
                          cwd=ROOT, capture_output=True)
    assert boot.returncode == 0, boot.stderr.decode()
    peer = subprocess.run([str(exe), "peer"], cwd=ROOT,
                          input=b'{"ops": []}', capture_output=True)
    assert peer.returncode == 0, peer.stderr.decode()
    # and asking for the absent vocabulary fails LOUDLY, naming it
    render = subprocess.run([str(exe), "render-graph", "1"], cwd=ROOT,
                            input=json.dumps(_hello()).encode(),
                            capture_output=True)
    assert render.returncode != 0 and \
        any(t in render.stderr for t in (b"osc", b"dac")), \
        (render.returncode, render.stderr[:200])


def pkg21_audio_package_no_behavior_change():
    # the audio package with no behavior change: region membership (dac
    # closure), latch/snapshot semantics, and offline pump re-run
    # UNCHANGED (EXE-2, EXE-4.1, EXE-6.2), and the package's declared
    # vocabulary is exactly what omission removes from the palette
    import rung05
    rung05.exe21_hello_regions()
    rung05.exe22_regions_recompute_per_edit()
    rung05.exe41_latch_at_block_start()
    rung05.exe62_pump_offline()
    pkgs = json.loads((ROOT / "vocabulary" / "packages.json").read_text())
    audio = set(pkgs["packages"]["audio"])
    full = set(json.loads(syg("palette")))
    omitted_exe = ROOT / "build-omit" / "syg"
    if not omitted_exe.exists():
        raise Pending("build-omit target absent (PKG-1.1 builds it)")
    omitted = set(json.loads(subprocess.run(
        [str(omitted_exe), "palette"], cwd=ROOT, capture_output=True).stdout))
    assert full - omitted == audio, \
        f"package shape drifted: {sorted((full - omitted) ^ audio)}"


def pkg41_gl_boundary_gate():
    # render_region is the GL boundary and the ONLY place that says GL.
    # The greenfield carries no GL yet — this gate STANDS so the first
    # gl*/EGL call outside the render package's region machinery fails
    # the suite the day it lands.
    hits = []
    for f in sorted((ROOT / "src").rglob("*")):
        if f.suffix not in {".cpp", ".hpp", ".c", ".h"}:
            continue
        if "render_region" in f.name:
            continue
        for i, ln in enumerate(f.read_text().splitlines(), 1):
            if re.search(r"\bgl[A-Z]\w*\s*\(", ln) or \
               re.search(r"\b(eglMakeCurrent|glesv?2?|GLES\d)\b", ln):
                hits.append(f"{f.relative_to(ROOT)}:{i}")
    assert not hits, f"GL escaped the render boundary: {hits}"
    # and the draw vocabulary belongs to the render package alone: every
    # type promising a draw/frame chain port is declared there
    pkgs = json.loads((ROOT / "vocabulary" / "packages.json").read_text())
    render = set(pkgs["packages"]["render"])
    chain_types = set()
    for d in (ROOT / "build" / "generated").glob("*.descriptor.json"):
        desc = json.loads(d.read_text())
        for pname, port in desc.get("ports", {}).items():
            if pname in ("tick", "chain", "frame") and \
               port.get("kind") == "bang" and port.get("discipline") == "event":
                chain_types.add(desc["type"])
    assert chain_types == render, \
        f"draw-chain vocabulary escaped the render package: {sorted(chain_types ^ render)}"


def pkg42_unchained_draw_does_not_render():
    # a draw node not wired into the head's chain does not render; the
    # chain propagates the SAME frame through wired draws, in order
    g = {"kind": "graph", "lock": {},
         "topology": {"nodes": {"head0": {"type": "render_head"},
                                "draw0": {"type": "instanced_draw"},
                                "draw1": {"type": "instanced_draw"},
                                "stray0": {"type": "instanced_draw"}},
                      "edges": [{"from": "head0/frame", "to": "draw0/tick"},
                                {"from": "draw0/chain", "to": "draw1/tick"}]},
         "defaults": {}}
    out = _exec_audit(g, blocks=130,
                      watch=["draw0/calls", "draw1/calls", "stray0/calls"])
    chained0 = out["watched"]["draw0/calls"][-1]
    chained1 = out["watched"]["draw1/calls"][-1]
    stray = out["watched"]["stray0/calls"][-1]
    assert chained0 >= 19, f"the head's chain never presented: {chained0}"
    assert chained1 == chained0, \
        f"the chain broke between draws: {chained0} vs {chained1}"
    # SAME-TICK propagation (ADR-015), not merely equal totals: the whole
    # watched series must be in lockstep, block by block — and note the
    # head sorts AFTER the draws here, so doc order cannot be the crutch
    assert out["watched"]["draw1/calls"] == out["watched"]["draw0/calls"], \
        "the chain lags a block behind the head"
    assert stray == 0.0, \
        f"an unchained draw rendered {stray} times — the chain is decorative"


def _exec_audit(graph, blocks=1, ops=None, watch=None):
    return json.loads(syg("exec-audit", stdin=json.dumps(
        {"graph": graph, "blocks": blocks, "ops": ops or [],
         "watch": watch or []}).encode()))


def pkg71_placement_is_fallthrough():
    # the same hello-cosine compiles with audio local and with audio
    # remote (the browser-peer environment of the worked example, here
    # supplied as the capability rule): diff of engine passes run — NONE;
    # only the adapter choice differs. No placement-specific pass exists.
    local = _peer([{"op": "set-app", "graph": _hello()}, {"op": "compile"}])[1]
    cap_splice = [
        {"op": "add_node", "a": "cap0", "b": "text_cell"},
        {"op": "set_text", "a": "cap0/value", "b": "remote:audio"},
        {"op": "add_edge", "a": "cap0/out", "b": "adapters0/in0"},
    ]
    remote = _peer([
        {"op": "set-app", "graph": _hello()},
        {"op": "open-engine-editor"},
        {"op": "engine-edit", "ops": cap_splice},
        {"op": "compile"},
    ])[3]
    # the ENGINE'S passes: same instances, same order — the capability
    # feed (cap0) is a rule source, not a pass
    engine_ids = set(json.loads(
        (ROOT / "vocabulary" / "engine-v0.json").read_text()
    )["topology"]["nodes"])
    p_local = [p for p in local["tick_order"] if p in engine_ids]
    p_remote = [p for p in remote["tick_order"] if p in engine_ids]
    assert set(p_local) == set(p_remote) == engine_ids, \
        f"a placement-specific pass appeared: {p_local} vs {p_remote}"
    # order is wiring in BOTH (the capability feed may legally reorder the
    # fan-in it joins; engine-v0's own precedence must hold either way)
    edges = json.loads((ROOT / "vocabulary" / "engine-v0.json").read_text()
                       )["topology"]["edges"]
    for order in (p_local, p_remote):
        pos = {p: i for i, p in enumerate(order)}
        for e in edges:
            f, t = e["from"].split("/")[0], e["to"].split("/")[0]
            if f in pos and t in pos:
                assert pos[f] < pos[t], (f, t, order)
    xl, xr = local["execution_body"], remote["execution_body"]
    # identical structure and map; only adapters + placement differ
    assert xl["map"] == xr["map"]
    assert xl["nodes"] == xr["nodes"]
    assert xl["placement"] == {"block": "local"}
    assert xr["placement"] == {"block": "remote"}
    ml = [m["mapping"] for m in xl["mappings"]]
    mr = [m["mapping"] for m in xr["mappings"]]
    assert "latch" in ml and "net:coalescable" in mr, (ml, mr)
    assert [m["edge"] for m in xl["mappings"]] == \
        [m["edge"] for m in xr["mappings"]], "the boundary walk itself moved"


def pkg51_worker_placement_by_capability():
    # Worker placement by ADVERTISED capability, over the mesh (MSH-3.1
    # dissolved the test-supplied capability table). A requester without the
    # worker capability schedules a heavy analysis derivation; only "linux"
    # advertises `render-analysis`, so placement lands there and the result
    # dataset comes back BY HASH (fetched, re-verified, then locally present).
    peers = {"quest": {}, "linux": {}}
    prelude = [
        {"op": "pair", "a": "quest", "b": "linux"},
        {"op": "advertise", "peer": "linux", "run": ["render-analysis"]},
    ]

    def mesh(ops):
        return json.loads(syg("mesh", stdin=json.dumps(
            {"peers": peers, "ops": prelude + ops}).encode()))["results"]

    r = mesh([
        {"op": "place-derivation", "from": "quest", "candidates": ["quest", "linux"],
         "graph": _chime(), "blocks": 200},
    ])
    placed = r[2]
    assert placed["ok"] and placed["placed_on"] == "linux", placed
    out_cid, take_cid = placed["output"], placed["take"]
    r = mesh([
        {"op": "place-derivation", "from": "quest", "candidates": ["quest", "linux"],
         "graph": _chime(), "blocks": 200},
        {"op": "has", "peer": "quest", "cid": out_cid},
        {"op": "fetch", "from": "quest", "to": "linux", "cid": out_cid},
        {"op": "has", "peer": "quest", "cid": out_cid},
        {"op": "fetch", "from": "quest", "to": "linux", "cid": take_cid},
        {"op": "has", "peer": "quest", "cid": take_cid},
        {"op": "read", "peer": "quest", "cid": out_cid},
    ])
    assert r[3]["has"] is False, "the requester had the result before fetching"
    assert r[4]["ok"] and r[5]["has"] is True
    assert r[7]["has"] is True, "the take dataset did not come back by hash"
    analysis = r[8]["node"]
    assert analysis.get("kind") == "analysis" and \
        analysis.get("rms_blocks") == 200, analysis
    # determinism: re-placing is a memo hit on the worker
    r2 = mesh([
        {"op": "place-derivation", "from": "quest", "candidates": ["linux"],
         "graph": _chime(), "blocks": 200},
        {"op": "place-derivation", "from": "quest", "candidates": ["linux"],
         "graph": _chime(), "blocks": 200},
    ])
    assert r2[3]["memo"] is True and r2[3]["output"] == out_cid
    # nobody home: no advertiser anywhere is a LOUD refusal
    import subprocess as sp
    exe = ROOT / "syg"
    rr = sp.run([str(exe), "mesh"], input=json.dumps(
        {"peers": peers, "ops": [
            {"op": "pair", "a": "quest", "b": "linux"},
            {"op": "place-derivation", "from": "quest",
             "candidates": ["quest", "linux"], "graph": _chime()}]}).encode(),
        capture_output=True)
    assert rr.returncode != 0 and b"worker" in rr.stderr


def pkg61_net_reconnect_discipline():
    # two-peer: consumer drives a cube from a provider lfo through a
    # proxy; kill and reconnect the link — event edges lose NOTHING (order
    # certified by payload), value edges resume AT LATEST (coalesced:
    # exactly one update spans the whole outage)
    provider = {"kind": "graph", "lock": {},
                "topology": {"nodes": {"lfo0": {"type": "lfo"}}, "edges": []},
                "defaults": {"lfo0/freq": 0.5, "lfo0/depth": 1.0}}
    consumer = {"kind": "graph", "lock": {},
                "topology": {"nodes": {"vfeed0": {"type": "button"},
                                       "proxy0": {"type": "net_proxy"},
                                       "cube0": {"type": "scale"},
                                       "efeed0": {"type": "button"},
                                       "counter0": {"type": "counter"}},
                             "edges": [{"from": "vfeed0/out", "to": "proxy0/in"},
                                       {"from": "proxy0/out", "to": "cube0/in"},
                                       {"from": "efeed0/out", "to": "counter0/in"}]},
                "defaults": {"cube0/k": 1.0}}
    # PKG-6 now runs over the MESH transport (MSH-7.1 dissolved the harness
    # link): the provider ships its flavored delivery log across the real
    # authenticated channel to a paired consumer.
    res = json.loads(syg("mesh", stdin=json.dumps(
        {"peers": {"prov": {}, "cons": {}}, "ops": [
            {"op": "pair", "a": "prov", "b": "cons"},
            {"op": "net-pair", "from": "prov", "to": "cons",
             "provider": provider, "consumer": consumer, "blocks": 300,
             "kill_at": 100, "reconnect_at": 200, "events": 150}]}).encode()))
    out = res["results"][1]
    # event edges lose nothing, in order (payloads 1..150 certify)
    assert out["count"] == 150.0, out
    assert out["disorder"] == 0.0, f"events reordered: {out}"
    # value edges resume at latest: the cube tracks the provider NOW
    assert abs(out["cube"] - out["provider_latest"]) < 1e-6, out
    # and the outage COALESCED: ~100 down blocks produced 1 update
    assert out["reconnect_value_posts"] == 1, out


def _tri_scene(stray=False):
    # fixtures/golden-frame.md: one red triangle through the head's chain;
    # optionally a SECOND triangle on a draw OUTSIDE the chain. v1
    # mesh_from_spans reads its positions from a serialized list default
    # (span-EDGE-fed geometry is a later succession — STRENGTHENINGS).
    nodes = {"head0": {"type": "render_head"},
             "mesh0": {"type": "mesh_from_spans"},
             "surf0": {"type": "surface_flat"},
             "draw0": {"type": "draw"}}
    edges = [{"from": "head0/frame", "to": "draw0/tick"},
             {"from": "mesh0/out", "to": "draw0/mesh"},
             {"from": "surf0/out", "to": "draw0/surface"}]
    defaults = {"mesh0/positions": [[-0.5, -0.5], [0.5, -0.5], [0.0, 0.5]],
                "surf0/r": 1.0, "surf0/g": 0.0, "surf0/b": 0.0,
                "surf0/a": 1.0, "mesh0/dx": 0.0, "mesh0/dy": 0.0}
    if stray:
        nodes.update({"mesh1": {"type": "mesh_from_spans"},
                      "stray0": {"type": "draw"}})
        edges += [{"from": "mesh1/out", "to": "stray0/mesh"},
                  {"from": "surf0/out", "to": "stray0/surface"}]
        defaults["mesh1/positions"] = [[0.6, 0.6], [0.9, 0.6], [0.75, 0.9]]
    return {"kind": "graph", "lock": {},
            "topology": {"nodes": nodes, "edges": edges},
            "defaults": defaults}


def _frames(req):
    from _helpers import frame_stats
    w, h = req["size"]
    raw = syg("frame", stdin=json.dumps(req).encode())
    n = req.get("frames", 1)
    assert len(raw) == n * w * h * 4, f"got {len(raw)} bytes for {n} frames"
    return [frame_stats(raw[i * w * h * 4:(i + 1) * w * h * 4], w, h)
            for i in range(n)]


def pkg43_pixels_are_real():
    from _helpers import ndc_to_px
    W = H = 128
    # analytic triangle: area 0.5 of NDC's 4.0 -> 1/8 coverage;
    # centroid NDC (0, -1/6)
    exp_cov = W * H / 8.0
    exp_cx, exp_cy = ndc_to_px(0.0, -1.0 / 6.0, W, H)
    # two frames; a set_param op between them translates the triangle
    # +0.4 NDC in x (golden-frame properties 1-4, 6)
    f0, f1 = _frames({"graph": _tri_scene(), "frames": 2, "size": [W, H],
                      "ops": [{"frame": 1, "route": "mesh0/dx",
                               "value": 0.4}]})
    assert abs(f0["coverage"] - exp_cov) <= 0.15 * exp_cov, \
        f"coverage {f0['coverage']} vs analytic {exp_cov}"
    assert abs(f0["cx"] - exp_cx) <= 0.03 * W and \
           abs(f0["cy"] - exp_cy) <= 0.03 * H, \
        f"centroid ({f0['cx']:.1f},{f0['cy']:.1f}) vs ({exp_cx:.1f},{exp_cy:.1f})"
    dx_px = f1["cx"] - f0["cx"]
    assert abs(dx_px - 0.2 * W) <= 0.2 * (0.2 * W), \
        f"param op moved centroid {dx_px:.1f}px, predicted {0.2 * W:.1f}px"
    assert abs(f1["cy"] - f0["cy"]) <= 0.03 * H, "y centroid drifted"
    assert len(f0["colors"]) <= 16, f"{len(f0['colors'])} distinct colors"
    # property 5: the stray (unchained) draw contributes ZERO coverage
    s0, = _frames({"graph": _tri_scene(stray=True), "frames": 1,
                   "size": [W, H]})
    assert s0["coverage"] == f0["coverage"], \
        f"unchained draw rendered: {s0['coverage']} vs {f0['coverage']} px"


def pkg44_shell_is_the_same_peer():
    # the shell is the ORDINARY peer: render executor presenting offscreen
    # under test; pointer input arrives ONLY as a package source node; a
    # pointer click bangs op_buttons whose ops land in the arbiter — the
    # SAME path every gesture graph uses (EDR-7, N4)
    from _helpers import frame_stats
    W = H = 128
    g = _tri_scene()
    g["topology"]["nodes"].update({
        "ptr0": {"type": "pointer"},
        "ob_add": {"type": "op_button"},
        "ob_wire": {"type": "op_button"},
        "k0": {"type": "cell"},
        "arb0": {"type": "arbiter_inlet"}})
    g["topology"]["edges"] += [
        {"from": "ptr0/click", "to": "ob_add/in"},
        {"from": "ptr0/click", "to": "ob_wire/in"},
        {"from": "ob_add/out", "to": "arb0/in"},
        {"from": "ob_wire/out", "to": "arb0/in"}]
    g["defaults"].update({
        "k0/k": 0.4,
        "ob_add/op": json.dumps({"op": "add_node", "a": "noise0",
                                 "b": "noise", "author": "pointer"}),
        "ob_wire/op": json.dumps({"op": "add_edge", "a": "k0/out",
                                  "b": "mesh0/dx", "author": "pointer"})})
    out = syg("shell", stdin=json.dumps(
        {"graph": g, "size": [W, H], "offscreen": True,
         "script": [{"frame": True},
                    {"pointer": {"x": 0.1, "y": 0.1, "buttons": 1}},
                    {"settle": True},
                    {"frame": True},
                    {"doc": True}]}).encode())
    fsz = W * H * 4
    f0 = frame_stats(out[:fsz], W, H)
    f1 = frame_stats(out[fsz:2 * fsz], W, H)
    doc = json.loads(out[2 * fsz:])
    # the gesture ADDED a node through the arbiter (persisted surface grew)
    assert "noise0" in doc["topology"]["nodes"], \
        sorted(doc["topology"]["nodes"])
    # and the wired cell moved the triangle: the next frame's centroid
    # shifted +0.4 NDC = +0.2*W px in x
    dx_px = f1["cx"] - f0["cx"]
    assert abs(dx_px - 0.2 * W) <= 0.2 * (0.2 * W), \
        f"pointer gesture moved centroid {dx_px:.1f}px, predicted {0.2 * W:.1f}"


TESTS = {
    "AUT-2.1": aut21_no_raw_frame_loops,
    "AUT-2.2": aut22_stamp_preserves_block_semantics,
    "AUT-5.1": aut51_four_routes_one_registry,
    "FRZ-1.1": frz11_ab_chime_interpreted_vs_frozen,
    "FRZ-1.2": frz12_unfreeze_is_reading_provenance,
    "FRZ-2.1": frz21_tier_derived_from_native_closure,
    "FRZ-3.1": frz31_arm_freestanding_link,
    "FRZ-4.1": None,
    "PKG-1.1": pkg11_package_omitted_at_build_time,
    "PKG-2.1": pkg21_audio_package_no_behavior_change,
    "PKG-3.1": None,
    "PKG-3.2": None,
    "PKG-3.3": None,  # ADR-037 + Quest in hand (embodiment plan, Phase E)
    "PKG-4.1": pkg41_gl_boundary_gate,
    "PKG-4.2": pkg42_unchained_draw_does_not_render,
    "PKG-4.3": pkg43_pixels_are_real,
    "PKG-4.4": pkg44_shell_is_the_same_peer,
    "PKG-5.1": pkg51_worker_placement_by_capability,
    "PKG-6.1": pkg61_net_reconnect_discipline,
    "PKG-7.1": pkg71_placement_is_fallthrough,
    "PKG-8.1": None,
}
