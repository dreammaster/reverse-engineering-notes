#!/usr/bin/env python3
"""
Struct-offset recovery helper. Given a global pointer variable name (e.g.
"playerchar"), scans rob_blanc_1.asm for every place a register is loaded
from it and then dereferenced with a byte-offset ("[reg+NNh]"), and prints
each occurrence with a few lines of surrounding context plus which function
it's in. Offsets are tallied by frequency so the most-used (highest value
to name first) show up at the top.

This does NOT try to guess field names -- it just surfaces the raw access
sites so a human (or a careful read of the calling function's already-
matched source counterpart) can assign meaning. See
reversing/notes/struct-layout-drift.md for the CharacterInfo/GUIMain work
this was built for.

Usage: python find_struct_accesses.py <global_name> [--context N]
"""
import re
import sys
import argparse
from pathlib import Path
from collections import defaultdict

ASM_PATH = Path(__file__).resolve().parent.parent.parent / "rob_blanc_1.asm"

PROC_RE = re.compile(r"^(\S+)\s+proc near\b")
ENDP_RE = re.compile(r"^(\S+)\s+endp\b")
MOV_FROM_GLOBAL_RE = None  # built per-invocation
OFFSET_RE = re.compile(r"\[(e[a-d]x|e[sd]i|ebp|esp)([+-][0-9A-Fa-f]+h)?\]")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("global_name")
    ap.add_argument("--context", type=int, default=1, help="lines of context after the mov to scan for [reg+off] use")
    ap.add_argument("--window", type=int, default=6, help="lines of context to print around each hit")
    args = ap.parse_args()

    mov_re = re.compile(r"^\s*mov\s+(e[a-d]x|e[sd]i),\s*" + re.escape(args.global_name) + r"\b")

    lines = ASM_PATH.read_text(encoding="utf-8", errors="replace").splitlines()

    current_func = None
    offset_hits = defaultdict(list)  # offset_int -> list of (line_no, func, snippet)

    for i, line in enumerate(lines):
        pm = PROC_RE.match(line)
        if pm:
            current_func = pm.group(1)
        em = ENDP_RE.match(line)
        if em:
            current_func = None

        m = mov_re.match(line)
        if not m:
            continue
        reg = m.group(1)
        # scan forward a few lines for [reg+NNh] or [reg-NNh]
        for j in range(i + 1, min(i + 1 + args.context + 3, len(lines))):
            off_m = re.search(r"\[" + re.escape(reg) + r"([+-][0-9A-Fa-f]+h?)?\]", lines[j])
            if off_m:
                off_str = off_m.group(1)
                if off_str is None:
                    offset = 0
                else:
                    sign = -1 if off_str[0] == '-' else 1
                    hexpart = off_str[1:].rstrip('h')
                    offset = sign * int(hexpart, 16)
                snippet = "\n".join(lines[max(0, i - 1):j + 2])
                offset_hits[offset].append((i + 1, current_func or "?", snippet))
                break

    print(f"Access sites for '{args.global_name}' ({sum(len(v) for v in offset_hits.values())} total, {len(offset_hits)} distinct offsets)\n")
    for offset in sorted(offset_hits.keys(), key=lambda o: -len(offset_hits[o])):
        hits = offset_hits[offset]
        funcs = sorted(set(h[1] for h in hits))
        print(f"=== offset {'+' if offset>=0 else ''}0x{offset:X} -- {len(hits)} site(s), funcs: {', '.join(funcs)} ===")


if __name__ == "__main__":
    main()
