"""
Read-only discovery: decode a Legacy of the Ancients client module's
`int 3Fh` run-time thunk table and list every entry with its call sites.

Legacy of the Ancients is compiled Microsoft BASIC 6.0. Every cross-module
call in a client .EXE (menu, out, dun, ...) is a `call far` into that
module's thunk segment (menu.idb: seg001), where each entry is a 3- or
4-byte trampoline:

    CD 3F nn          int 3Fh ; db nn          -> run-time entry #nn       (nn = 00..FD)
    CD 3F FF nn       int 3Fh ; db FF ; db nn  -> run-time entry #FF nn
    CD 3F FE nn       int 3Fh ; db FE ; db nn  -> run-time entry #FE nn

FE / FF are always prefixes, never bare ordinals. The int 3Fh handler is
installed by LEGLIB.EXE (the shared run-time module); it resolves the
(prefix, ordinal) key to a real segment:offset, transfers control, and the
callee `retf`s straight back to the client's original caller -- so the
thunk itself never returns and the bytes after it are the *next* thunk,
not inline data.

The (prefix, ordinal) key is stable across every client module (they all
link against the same LEGLIB), so a name learned for one applies to all.

Usage (read-only):
    .\run_ida_script.ps1 -Idb menu -ScriptName dump_thunk_table.py -NoExport
"""

import idc
import idautils
import ida_bytes
import ida_segment

# Which segment holds the thunk table. menu.idb = seg001; override per
# module as new IDBs are added.
THUNK_SEG = "seg001"


def key_name(prefix, ordinal):
    if prefix is None:
        return f"rt_{ordinal:02X}"
    return f"rt_{prefix:02X}{ordinal:02X}"


def main():
    seg = ida_segment.get_segm_by_name(THUNK_SEG)
    if seg is None:
        print(f"no segment {THUNK_SEG!r}")
        return

    entries = []  # (ea, prefix, ordinal, size)
    ea = seg.start_ea
    while ea < seg.end_ea - 2:
        if ida_bytes.get_byte(ea) == 0xCD and ida_bytes.get_byte(ea + 1) == 0x3F:
            b2 = ida_bytes.get_byte(ea + 2)
            if b2 in (0xFE, 0xFF):
                prefix, ordinal, size = b2, ida_bytes.get_byte(ea + 3), 4
            else:
                prefix, ordinal, size = None, b2, 3
            entries.append((ea, prefix, ordinal, size))
            ea += size
        else:
            ea += 1

    print(f"{THUNK_SEG}: {seg.start_ea:#x}-{seg.end_ea:#x}, {len(entries)} thunks\n")

    rows = []
    for ea, prefix, ordinal, size in entries:
        refs = sorted(set(idautils.CodeRefsTo(ea, 0)) | set(idautils.DataRefsTo(ea)))
        callers = {}
        for r in refs:
            fn = idc.get_func_name(r) or f"loc_{r:X}"
            callers[fn] = callers.get(fn, 0) + 1
        rows.append((len(refs), ea, prefix, ordinal, size, callers))

    # keyed summary (prefix, ordinal) -> total refs
    by_key = {}
    for nref, ea, prefix, ordinal, size, callers in rows:
        by_key.setdefault((prefix, ordinal), [0, []])
        by_key[(prefix, ordinal)][0] += nref
        by_key[(prefix, ordinal)][1].append(ea)

    dup = {k: v for k, v in by_key.items() if len(v[1]) > 1}
    if dup:
        print(f"[!] {len(dup)} (prefix,ordinal) keys have >1 thunk EA -- "
              f"table is NOT a flat namespace, or scan is misaligned:")
        for (p, o), (n, eas) in sorted(dup.items()):
            print(f"    {key_name(p, o)}: {', '.join(f'{e:#x}' for e in eas)}")
        print()

    prefixes = {}
    for (p, o) in by_key:
        prefixes.setdefault(p, []).append(o)
    for p in sorted(prefixes, key=lambda x: (x is not None, x)):
        os_ = sorted(prefixes[p])
        label = "(bare)" if p is None else f"{p:#04x}"
        print(f"prefix {label}: {len(os_)} ordinals  {os_[0]:#04x}..{os_[-1]:#04x}")
    print()

    rows.sort(key=lambda r: -r[0])
    print("most-referenced thunks (top 40):")
    for nref, ea, prefix, ordinal, size, callers in rows[:40]:
        cs = ", ".join(f"{k}×{v}" if v > 1 else k
                       for k, v in sorted(callers.items(), key=lambda kv: -kv[1])[:5])
        print(f"  {key_name(prefix, ordinal):9s} @ {ea:#07x}  refs={nref:3d}  <- {cs}")

    print(f"\ntotal distinct run-time entries used by this module: {len(by_key)}")
    unref = [key_name(p, o) for (p, o), (n, _) in by_key.items() if n == 0]
    print(f"never referenced (dead thunks): {len(unref)}")


main()
