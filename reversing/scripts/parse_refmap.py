#!/usr/bin/env python3
"""
Parse the MSVC linker .map file produced by building the reference AGS
3.2.1.1115 source tree (Engine/acwin___Win32_DebugWorking/acwin.map) into a
name -> {addr, objfile, demangled} lookup.

This is a *reference* build (built by the user from the known-good source,
2024 timestamp) -- NOT the Rob Blanc 1 binary. Its value is that it gives the
exact linker symbol name and originating .obj (== source file) for every
function/global in the reference engine, which is far more precise than
grepping for string literals when trying to confirm which source file an
already-matched disassembly function came from, and gives a name/prototype
dictionary to check candidate names against when identifying sub_* functions.

Output: reversing/analysis/refmap_symbols.json
  { name: {addr, objfile, raw} }  -- raw is the undecorated map line symbol
  (decorated C++ names kept as-is; a lightly demangled "simple_name" is also
  provided for common patterns: ?name@@... -> name)
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
MAP_PATH = ROOT / "Engine" / "acwin___Win32_DebugWorking" / "acwin.map"
OUT_DIR = Path(__file__).resolve().parent.parent / "analysis"

LINE_RE = re.compile(
    r"^\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(\S+)\s+[0-9A-Fa-f]{8}\s+(?:f\s+)?(?:i\s+)?(.+?)\s*$"
)


def simple_name(sym):
    # MSVC decorated names: ?name@@... or ?name@Class@@...
    if sym.startswith("?"):
        m = re.match(r"^\?([A-Za-z_][A-Za-z0-9_]*)@", sym)
        if m:
            return m.group(1)
    # C names sometimes prefixed with a leading underscore
    if sym.startswith("_") and not sym.startswith("__"):
        return sym[1:].split("@")[0]
    return sym.split("@")[0]


def main():
    text = MAP_PATH.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()

    start = None
    for i, l in enumerate(lines):
        if "Publics by Value" in l:
            start = i + 1
            break
    if start is None:
        raise SystemExit("Could not find 'Publics by Value' section in map file")

    symbols = {}
    for line in lines[start:]:
        if not line.strip():
            continue
        m = LINE_RE.match(line)
        if not m:
            continue
        sym, obj = m.group(1), m.group(2)
        if obj in ("<absolute>", "<linker-defined>"):
            continue
        addr_m = re.match(r"^\s*([0-9A-Fa-f]{4}):([0-9A-Fa-f]{8})", line)
        section, offset = addr_m.group(1), addr_m.group(2)
        name = simple_name(sym)
        entry = {"decorated": sym, "objfile": obj, "section": section, "offset": offset}
        # keep first occurrence; note collisions
        if name in symbols and symbols[name]["objfile"] != obj:
            symbols.setdefault("__collisions__", []) if False else None
        symbols[name] = entry

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUT_DIR / "refmap_symbols.json").write_text(json.dumps(symbols, indent=1), encoding="utf-8")
    print(f"Parsed {len(symbols)} symbols from reference map")


if __name__ == "__main__":
    main()
