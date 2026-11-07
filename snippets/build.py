#!/usr/bin/env python3
"""Aggregate snippets/*.cpp into ~/.config/nvim/UltiSnips/cpp.snippets.

One file per snippet: the file name is the trigger (seg.cpp -> `seg`), the first line is
`// description`, the rest is the body. UltiSnips placeholders ($0, ${1:x}) are allowed.
"""
from pathlib import Path

SRC = Path(__file__).resolve().parent
OUT = Path.home() / ".config/nvim/UltiSnips/cpp.snippets"

parts = []
for f in sorted(SRC.glob("*.cpp")):
    first, _, body = f.read_text().partition("\n")
    if not first.startswith("//"):
        print(f"skipped {f.name}: first line must be '// description'")
        continue
    desc = first[2:].strip().replace('"', "'")
    parts.append(f'snippet {f.stem} "{desc}"\n{body.rstrip()}\nendsnippet\n')
OUT.write_text("\n".join(parts))
print(f"{len(parts)} snippets -> {OUT}")
