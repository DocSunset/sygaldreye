#!/usr/bin/env python3
"""Extract the requirements manifest from the architecture book.

Regenerate with:  python3 conformance/extract_manifest.py > conformance/manifest.json
The manifest is the index cnf.suite_as_data requires: every requirement and acceptance
criterion, with its chapter, text, and build rung. If the book and the
manifest disagree, regenerate the manifest — never edit it by hand.
"""
import json, re, sys, pathlib

BOOK = pathlib.Path(__file__).resolve().parent.parent / "architecture"

# Build-rung assignment (Appendix A). Default by prefix; overrides by id.
RUNG_BY_PREFIX = {
    "fmt": 1, "nam": 1,
    "cor": 2, "abi": 2, "aut": 2,
    "sz": 4, "lng": 5, "exe": 5, "tcf": 5,
    "sto": 6, "cmp": 7, "frz": 8, "pkg": 8,
    "msh": 9, "edr": 11, "cnf": 12,
}
RUNG_OVERRIDES = {
    "cor.crown_sufficiency": 3, "cor.two_profiles": 12, "fmt.boot_tape": 3, "fmt.wire_transcripts": 9,
    "sz.trampolines": 2, "sz.platform_invariance": 2, "exe.migration": 4, "lng.round_trip": 4, "lng.query_vocabulary": 6,
    "abi.contract_succession": 9, "abi.three_packagings": 10, "sto.refs_and_undo": 6, "cmp.retired": None,  # retired tombstone
    "lng.structured_payloads": 7,  # ADR-034: the structured lane is rung 7's opening move
    # AUT was missing from the prefix map — ch. 12 was silently absent from
    # the manifest (found 2026-07-05). Kernels/generator bind at rung 2,
    # lifting at rung 5, shells-over-ugens and the four routes at rung 8.
    "aut.shell_stamps": 8, "aut.lift_guarantees": 5, "aut.four_routes": 8,
}

# Identifiers are namespaced slugs now (ADR-039): `**nam.addresses**` heads a
# requirement; `- nam.addresses.parse_print_roundtrip:` heads a criterion. The
# family alternation keeps stray dotted bold (`**adr.md**`) from matching.
FAM = "|".join(RUNG_BY_PREFIX)
req_pat = re.compile(rf"\*\*(({FAM})\.[a-z0-9_]+)\*\*\s*(.*)")
ac_pat = re.compile(rf"^\s*-\s+(({FAM})\.[a-z0-9_]+\.[a-z0-9_]+):\s*(.*)")

manifest = {}
for f in sorted(BOOK.glob("[0-9][0-9]-*.md")):
    chapter = f.name
    cur = None
    for line in f.read_text().split("\n"):
        m = req_pat.match(line.strip())
        if m:
            rid, prefix, rest = m.group(1), m.group(2), m.group(3)
            rung = RUNG_OVERRIDES.get(rid, RUNG_BY_PREFIX[prefix])
            manifest[rid] = {"chapter": chapter, "name": rid,
                             "text": rest.strip(), "rung": rung, "criteria": {}}
            cur = rid
            continue
        m = ac_pat.match(line)
        if m:
            acid, text = m.group(1), m.group(3)
            parent = acid.rsplit(".", 1)[0]
            if parent in manifest:
                manifest[parent]["criteria"][acid] = text.strip()

json.dump(manifest, sys.stdout, indent=1, sort_keys=True)
print()
print(f"{len(manifest)} requirements, "
      f"{sum(len(r['criteria']) for r in manifest.values())} acceptance criteria",
      file=sys.stderr)
