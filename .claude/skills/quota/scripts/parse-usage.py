#!/usr/bin/env python3
import re, sys

text = sys.stdin.read()

def block(header):
    m = re.search(rf"{re.escape(header)}\s*\n[^\n]+?(\d+)%\s*used\s*\n\s*Resets\s+([^\n]+)", text)
    return (int(m.group(1)), m.group(2).strip()) if m else None

def credits():
    m = re.search(r"Usage credits\s*\n[^\n]+?(\d+)%\s*used\s*\n([^\n]+?)\s*·\s*Resets\s+([^\n]+)", text)
    return (int(m.group(1)), m.group(2).strip(), m.group(3).strip()) if m else None

s = block("Current session")
w = block("Current week (all models)")
c = credits()

if not any([s, w, c]):
    print(text)
    sys.exit(1)

if s: print(f"Session:  {s[0]}% used  —  resets {s[1]}")
if w: print(f"Week:     {w[0]}% used  —  resets {w[1]}")
if c: print(f"Credits:  {c[0]}% used  ({c[1]})  —  resets {c[2]}")
