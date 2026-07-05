#!/usr/bin/env python3
"""Extract the requirements manifest from the architecture book.

Regenerate with:  python3 conformance/extract_manifest.py > conformance/manifest.json
The manifest is the index CNF-1 requires: every requirement and acceptance
criterion, with its chapter, text, and build rung. If the book and the
manifest disagree, regenerate the manifest — never edit it by hand.
"""
import json, re, sys, pathlib

BOOK = pathlib.Path(__file__).resolve().parent.parent / "architecture"

# Build-rung assignment (Appendix A). Default by prefix; overrides by id.
RUNG_BY_PREFIX = {
    "FMT": 1, "NAM": 1,
    "COR": 2, "ABI": 2, "AUT": 2,
    "SZ": 4, "LNG": 5, "EXE": 5, "TCF": 5,
    "STO": 6, "CMP": 7, "FRZ": 8, "PKG": 8,
    "MSH": 9, "EDR": 11, "CNF": 12,
}
RUNG_OVERRIDES = {
    "COR-2": 3, "COR-4": 12, "FMT-3": 3, "FMT-4": 9,
    "SZ-6": 2, "SZ-1": 2, "EXE-5": 4, "LNG-8": 4, "LNG-10": 6,
    "ABI-4": 9, "ABI-5": 10, "STO-4": 6, "CMP-8": None,  # retired tombstone
    "LNG-11": 7,  # ADR-034: the structured lane is rung 7's opening move
    # AUT was missing from the prefix map — ch. 12 was silently absent from
    # the manifest (found 2026-07-05). Kernels/generator bind at rung 2,
    # lifting at rung 5, shells-over-ugens and the four routes at rung 8.
    "AUT-2": 8, "AUT-4": 5, "AUT-5": 8,
}

req_pat = re.compile(r"\*\*([A-Z]{2,3}-\d+)(?:\s*\(([^)]*)\))?\.?\*\*\s*(.*)")
ac_pat = re.compile(r"^\s*-\s+([A-Z]{2,3}-\d+\.\d+):\s*(.*)")

manifest = {}
for f in sorted(BOOK.glob("[0-9][0-9]-*.md")):
    chapter = f.name
    cur = None
    for line in f.read_text().split("\n"):
        m = req_pat.match(line.strip())
        if m and "-" in m.group(1):
            rid, name, rest = m.group(1), m.group(2) or "", m.group(3)
            prefix = rid.split("-")[0]
            if prefix not in RUNG_BY_PREFIX:
                continue
            rung = RUNG_OVERRIDES.get(rid, RUNG_BY_PREFIX[prefix])
            manifest[rid] = {"chapter": chapter, "name": name,
                             "text": rest.strip(), "rung": rung, "criteria": {}}
            cur = rid
            continue
        m = ac_pat.match(line)
        if m:
            acid, text = m.group(1), m.group(2)
            parent = acid.rsplit(".", 1)[0]
            if parent in manifest:
                manifest[parent]["criteria"][acid] = text.strip()

json.dump(manifest, sys.stdout, indent=1, sort_keys=True)
print()
print(f"{len(manifest)} requirements, "
      f"{sum(len(r['criteria']) for r in manifest.values())} acceptance criteria",
      file=sys.stderr)
