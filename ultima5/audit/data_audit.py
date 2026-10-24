"""
Initialised-data audit: compare every initialised global in src/vars.c with the original DATA.OVL image.

DATA.OVL is the program's DGROUP image with a 0x10-byte header (DS offset = file offset - 0x10).  We generate a tiny C
program that links vars.c, dumps (address, size, bytes) for every non-pointer D_xxxx variable declared in vars.h, and
diff those bytes against DATA.OVL.  String-table entries (_STATIC_TEXT(addr,"...")) are checked separately against
the string bytes at that address.

Writes audit/data_audit.md
"""
import json
import os
import re
import struct
import subprocess
import sys
import tempfile

import audit_functions as a

DATA = os.path.join(a.GAME_DIR, "DATA.OVL")
HDR = 0x10
SRC = a.SRC.replace(chr(92), "/")


def parse_decls():
    decls = []
    for line in open(os.path.join(SRC, "vars.h"), encoding="utf-8", errors="replace"):
        m = re.match(r"^STUB\s+(.+?)\s+(D_([0-9a-f]{4})\w*)((?:\[[^\]]*\])*)\s*;", line)
        if not m:
            continue
        typ, name, addr, dims = m.group(1), m.group(2), int(m.group(3), 16), m.group(4)
        decls.append((typ, name, addr, dims))
    return decls


def main():
    decls = parse_decls()
    ptr = [d for d in decls if "*" in d[0]]
    plain = [d for d in decls if "*" not in d[0]]
    tmp = tempfile.mkdtemp(prefix="u5data_")
    gen = os.path.join(tmp, "dump.c")
    with open(gen, "w") as f:
        f.write('#include "common/common.h"\n#include "vars.h"\n#include <stdio.h>\n')
        f.write("typedef struct { unsigned addr; unsigned size; const unsigned char* p; } E;\n")
        f.write("static const E tab[] = {\n")
        addrs = sorted(set(d[2] for d in decls))
        for typ, name, addr, dims in plain:
            if "[]" in dims:
                nxt = [x for x in addrs if x > addr]
                f.write('  {0x%x, %d, (const unsigned char*)&%s},\n' % (addr, (nxt[0] if nxt else 0x10000) - addr, name))
            else:
                f.write('  {0x%x, (unsigned)sizeof(%s), (const unsigned char*)&%s},\n' % (addr, name, name))
        f.write("};\nint main(void){ FILE*o=fopen(\"%s\",\"wb\"); for(unsigned i=0;i<sizeof(tab)/sizeof(tab[0]);i++){ fwrite(&tab[i].addr,4,1,o); fwrite(&tab[i].size,4,1,o); fwrite(tab[i].p,1,tab[i].size,o);} fclose(o); return 0; }\n"
                % os.path.join(tmp, "dump.bin").replace("\\", "/"))
    exe = os.path.join(tmp, "dump.exe")
    cmd = ["gcc", "-std=gnu11", "-w", "-I" + SRC, "-DU5_BACKEND_WIN32", "-fno-strict-aliasing", gen, os.path.join(SRC, "vars.c"),
           "-o", exe, "-Wl,--unresolved-symbols=ignore-all"]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stderr[-2000:])
        sys.exit(1)
    subprocess.run([exe], check=True)
    blob = open(os.path.join(tmp, "dump.bin"), "rb").read()
    data = open(DATA, "rb").read()

    pos = 0
    rows = []
    for typ, name, addr, dims in plain:
        a_, size = struct.unpack_from("<II", blob, pos)
        pos += 8
        mine = blob[pos:pos + size]
        pos += size
        # only the ranges that are present in the file image
        ref = data[HDR + addr:HDR + addr + size]
        rows.append((name, typ, dims, addr, size, mine, ref))

    ok = bad = zero = 0
    md = ["# Initialised data audit (vars.c vs DATA.OVL)\n\n"]
    md.append("%d non-pointer variables compared; %d pointer tables (string/pointer arrays) skipped here and checked via the "
              "string audit below.\n\n" % (len(plain), len(ptr)))
    details = []
    for name, typ, dims, addr, size, mine, ref in rows:
        if len(ref) < size:
            details.append((name, addr, size, "beyond DATA.OVL image"))
            continue
        if mine == ref:
            ok += 1
            continue
        if not any(mine):
            zero += 1
            # C side has no initialiser: original has non-zero data here?
            if any(ref):
                details.append((name, addr, size, "C uninitialised/zero but DATA.OVL has non-zero data (%d non-zero bytes)" % sum(1 for x in ref if x)))
            else:
                ok += 1
            continue
        bad += 1
        diffs = [i for i in range(size) if mine[i] != ref[i]]
        details.append((name, addr, size, "%d/%d bytes differ; first at +0x%x (C %02x vs orig %02x)" % (len(diffs), size, diffs[0], mine[diffs[0]], ref[diffs[0]])))

    md.append("**Result:** %d identical, %d differ, %d zero in C while non-zero in the original.\n\n" % (ok, bad, sum(1 for d in details if "zero but" in d[3] or "uninitialised" in d[3])))
    md.append("| var | DS addr | size | finding |\n|---|---|---|---|\n")
    for name, addr, size, msg in details:
        md.append("| %s | %04x | %d | %s |\n" % (name, addr, size, msg))

    # ---- strings ---------------------------------------------------------------------------------------
    text = []
    for fn in sorted(os.listdir(SRC)):
        if fn.endswith(".c"):
            t = open(os.path.join(SRC, fn), encoding="utf-8", errors="replace").read()
            for m in re.finditer(r'_(?:STATIC_)?TEXT\(\s*0x([0-9a-fA-F]{1,4})\s*,\s*((?:"(?:[^"\\]|\\.)*"\s*)+)\)', t):
                addr = int(m.group(1), 16)
                lits = re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(2))
                s = "".join(lits)
                s = bytes(s, "utf-8").decode("unicode_escape").encode("latin-1", "replace")
                text.append((fn, addr, s))
    bads = []
    nok = 0
    for fn, addr, s in text:
        ref = data[HDR + addr:HDR + addr + len(s)]
        if ref == s:
            nok += 1
        else:
            bads.append((fn, addr, s, ref))
    md.append("\n## String literals (%d `_TEXT/_STATIC_TEXT` with a DS address)\n\n%d match DATA.OVL exactly; %d differ.\n\n" % (len(text), nok, len(bads)))
    if bads:
        md.append("| file | addr | C string | original bytes |\n|---|---|---|---|\n")
        for fn, addr, s, ref in bads[:300]:
            md.append("| %s | %04x | `%s` | `%s` |\n" % (fn, addr, s.decode("latin-1").encode("unicode_escape").decode(), ref.decode("latin-1").encode("unicode_escape").decode()))
    open(os.path.join(a.ROOT, "audit", "data_audit.md"), "w").write("".join(md))
    print("vars: ok=%d differ=%d other=%d; strings: ok=%d differ=%d" % (ok, bad, len(details) - bad, nok, len(bads)))


if __name__ == "__main__":
    main()
