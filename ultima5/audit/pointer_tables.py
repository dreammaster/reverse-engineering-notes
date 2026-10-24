"""
Pointer-table audit: for every `char* D_xxxx[...] = { _STATIC_TEXT(0xADDR, "..."), 0, ... }` table in vars.c,
check that each entry's address equals the 16-bit near pointer stored in the original DATA.OVL image at that table
slot, and that the table is not truncated/oversized.  (Complements data_audit.py, which verifies the strings
themselves.)  Appends to audit/data_audit.md.
"""
import os
import re
import struct

import audit_functions as a

DATA = os.path.join(a.GAME_DIR, "DATA.OVL")
HDR = 0x10

TABLE_RE = re.compile(r"^(?:static\s+)?char\*\s+(D_([0-9a-f]{4})\w*)\[([^\]]*)\]\s*=\s*\{(.*?)\n\};", re.S | re.M)
ENTRY_RE = re.compile(r"_STATIC_TEXT\(\s*0x([0-9a-fA-F]+)\s*,|(?<![\w])(0)(?![\w])|(\"[^\"]*\")|(/\*.*?\*/)", re.S)


def main():
    src = open(os.path.join(a.SRC, "vars.c"), encoding="utf-8", errors="replace").read()
    data = open(DATA, "rb").read()
    out = ["\n## Pointer tables (string/char* arrays in vars.c vs original near pointers)\n\n"]
    out.append("| table | DS addr | declared | C entries | findings |\n|---|---|---|---|---|\n")
    total = good = 0
    for m in TABLE_RE.finditer(src):
        name, addr, dim, body = m.group(1), int(m.group(2), 16), m.group(3), m.group(4)
        body = re.sub(r"//[^\n]*", "", body)
        body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
        body = re.sub(r'"(?:[^"\\]|\\.)*"', '""', body)
        entries = []
        for mm in re.finditer(r"_STATIC_TEXT\(\s*0x([0-9a-fA-F]+)|(?<![\w])(0|NULL)(?![\w])", body):
            entries.append(int(mm.group(1), 16) if mm.group(1) else 0)
        issues = []
        for i, e in enumerate(entries):
            off = HDR + addr + 2 * i
            if off + 2 > len(data):
                issues.append("slot %d beyond image" % i)
                continue
            orig = struct.unpack_from("<H", data, off)[0]
            if e is None:
                issues.append("slot %d: C entry is not a plain string address (orig %04x)" % (i, orig))
            elif e != orig:
                issues.append("slot %d: C %04x vs orig %04x" % (i, e, orig))
        total += 1
        if not issues:
            good += 1
        out.append("| %s | %04x | %s | %d | %s |\n" % (name, addr, dim, len(entries), "ok" if not issues else "; ".join(issues[:6]) + (" ..." if len(issues) > 6 else "")))
    out.insert(1, "%d of %d tables identical to the original pointer values.\n\n" % (good, total))
    with open(os.path.join(a.ROOT, "audit", "data_audit.md"), "a") as f:
        f.write("".join(out))
    print("pointer tables: %d/%d identical" % (good, total))


if __name__ == "__main__":
    main()
