#!/usr/bin/env python3
"""
UCSD p-code disassembler for the DOS Wizardry SYSTEM.PASCAL codefile.

Dialect and operand formats are from docs/pmachine.md (recovered from
SYSTEM.INTERP). Codefile structure (all little-endian, no byte-swap):

  block 0            codefile directory
      +0x00  16 x (u16 firstBlock, u16 byteLength)   segment table
      +0x40  16 x  8-byte segment name
  segment            raw p-code starting at offset 0; at the very end:
      last word      lo byte = p-System segment number, hi byte = procedure count
      before it      <nproc> u16 self-relative pointers (proc 1..nproc), proc p
                     pointer at  segEnd-2-2*p ; PAT anchor = ptrPos - *ptrPos
  PAT (at anchor)    [anchor-8] u16 data size (local bytes)
                     [anchor-6] u16 param size (bytes)
                     [anchor-4] u16 exit-IC self-rel (points near proc end)
                     [anchor-2] u16 entry-IC self-rel  -> first bytecode
                     [anchor]   u16 procedure number (sanity check)
  Procedure code     [entry, anchor-8) ; a self-relative jump table may sit at
                     the tail of that range (backward UJP/FJP with a negative
                     byte index a word table just below the PAT anchor).

Jumps:
  FJP/UJP  1 signed byte d. d>=0 (i.e. <0x80): IP += d (forward, from after the
           operand). d<0 (>=0x80): target = word@(PATanchor+d), self-relative.
  XJP      word-align IP; i16 min, i16 max; (max-min+1) self-relative words;
           then the default falls through past the table.

Usage:
    python tools/pcode_dis.py segments <SYSTEM.PASCAL>
    python tools/pcode_dis.py procs    <SYSTEM.PASCAL> <seg>
    python tools/pcode_dis.py dis      <SYSTEM.PASCAL> <seg> [proc]
    python tools/pcode_dis.py dis-all  <SYSTEM.PASCAL> <outdir>

<seg> is a segment name (WIZARDRY, COMBAT, ...) or 0-based table index.
"""

import os
import sys
import struct

BLOCK = 512

# p-System segment number -> name, filled from the codefile itself.
SEGNUM_NAME = {}

# (segname, procnum) -> apple name, loaded from docs/procmap.tsv if present.
PROCMAP = {}
CUR_SEG = ""          # segment being disassembled (for CLP/CGP/CIP/CBP targets)


def load_procmap(path="docs/procmap.tsv"):
    if not os.path.exists(path):
        return
    for line in open(path):
        if line.startswith("#") or "\t" not in line:
            continue
        f = line.rstrip("\n").split("\t")
        if len(f) >= 3:
            PROCMAP[(f[0], int(f[1]))] = f[2]


def pname(segname, procnum):
    return PROCMAP.get((segname, procnum))


GLOBALS = {}          # word -> name, from docs/globals.tsv
GRANGES = [(363, 987, "CHARACTR", 104), (987, 1239, "SCNTOC", None),
           (1239, 1751, "IOCACHE", None)]


def load_globals(path="docs/globals.tsv"):
    if not os.path.exists(path):
        return
    for line in open(path):
        if line.startswith("#") or "\t" not in line:
            continue
        f = line.rstrip("\n").split("\t")
        if len(f) >= 2:
            GLOBALS[int(f[0])] = f[1]


def gname(w):
    if w in GLOBALS:
        return GLOBALS[w]
    for lo, hi, nm, stride in GRANGES:
        if lo <= w < hi:
            return f"{nm}+{w - lo}" if stride is None else f"{nm}[{(w-lo)//stride}]+{(w-lo)%stride}"
    return None

# opcode -> (mnemonic, operand-kind)
OPS = {
    0x80: ("ABI", "-"), 0x81: ("ABR", "-"), 0x82: ("ADI", "-"), 0x83: ("ADR", "-"),
    0x84: ("LAND", "-"), 0x85: ("DIF", "-"), 0x86: ("DVI", "-"), 0x87: ("DVR", "-"),
    0x88: ("CHK", "-"), 0x89: ("FLO", "-"), 0x8A: ("FLT", "-"), 0x8B: ("INN", "-"),
    0x8C: ("INT", "-"), 0x8D: ("LOR", "-"), 0x8E: ("MODI", "-"), 0x8F: ("MPI", "-"),
    0x90: ("MPR", "-"), 0x91: ("NGI", "-"), 0x92: ("NGR", "-"), 0x93: ("LNOT", "-"),
    0x94: ("SRS", "-"), 0x95: ("SBI", "-"), 0x96: ("SBR", "-"), 0x97: ("SGS", "-"),
    0x98: ("SQI", "-"), 0x99: ("SQR", "-"), 0x9A: ("STO", "-"), 0x9B: ("IXS", "-"),
    0x9C: ("UNI", "-"), 0x9D: ("LDE", "seg_big"), 0x9E: ("CSP", "csp"), 0x9F: ("LDCN", "-"),
    0xA0: ("ADJ", "ub"), 0xA1: ("FJP", "jump"), 0xA2: ("INC", "big"), 0xA3: ("IND", "big"),
    0xA4: ("IXA", "big"), 0xA5: ("LAO", "big"), 0xA6: ("LSA", "str"), 0xA7: ("LAE", "seg_big"),
    0xA8: ("MOV", "big"), 0xA9: ("LDO", "big"), 0xAA: ("SAS", "ub"), 0xAB: ("SRO", "big"),
    0xAC: ("XJP", "xjp"), 0xAD: ("RNP", "ub"), 0xAE: ("CIP", "ub"), 0xAF: ("EQU", "ubt"),
    0xB0: ("GEQ", "ubt"), 0xB1: ("GTR", "ubt"), 0xB2: ("LDA", "ll_big"), 0xB3: ("LDC", "ldc"),
    0xB4: ("LEQ", "ubt"), 0xB5: ("LES", "ubt"), 0xB6: ("LOD", "ll_big"), 0xB7: ("NEQ", "ubt"),
    0xB8: ("STR", "ll_big"), 0xB9: ("UJP", "jump"), 0xBA: ("LDP", "-"), 0xBB: ("STP", "-"),
    0xBC: ("LDM", "ub"), 0xBD: ("STM", "ub"), 0xBE: ("LDB", "-"), 0xBF: ("STB", "-"),
    0xC0: ("IXP", "ub_ub"), 0xC1: ("RBP", "ub"), 0xC2: ("CBP", "ub"), 0xC3: ("EQUI", "-"),
    0xC4: ("GEQI", "-"), 0xC5: ("GTRI", "-"), 0xC6: ("LLA", "big"), 0xC7: ("LDCI", "word"),
    0xC8: ("LEQI", "-"), 0xC9: ("LESI", "-"), 0xCA: ("LDL", "big"), 0xCB: ("NEQI", "-"),
    0xCC: ("STL", "big"), 0xCD: ("CXP", "cxp"), 0xCE: ("CLP", "ub"), 0xCF: ("CGP", "ub"),
    0xD0: ("LPA", "str"), 0xD1: ("STE", "seg_big"), 0xD2: ("NOP", "-"), 0xD3: ("BPT?", "-"),
    0xD4: ("BPT?", "-"), 0xD5: ("SKIP1", "big"), 0xD6: ("HLT", "-"), 0xD7: ("NOP", "-"),
}
CSP_NAMES = {0: "csp0", 1: "NEW", 2: "MOVELEFT", 3: "MOVERIGHT", 4: "EXIT",
             5: "UNITREAD", 6: "UNITWRITE", 7: "csp7", 8: "TREESRCH?", 9: "csp9",
             10: "FILLCHAR", 11: "SCAN", 12: "csp12", 21: "csp21", 22: "csp22",
             23: "csp23", 24: "csp24", 32: "MARK", 33: "RELEASE"}
CMP_TYPE = {2: "REAL", 4: "STR", 6: "BOOL", 8: "SET", 10: "BYTE", 12: "WORD"}


def rd16(b, o):
    return struct.unpack_from("<H", b, o)[0]


def rd16s(b, o):
    return struct.unpack_from("<h", b, o)[0]


class Proc:
    def __init__(self, num, anchor, entry, dsize, psize, exitrel):
        self.num, self.anchor, self.entry = num, anchor, entry
        self.dsize, self.psize, self.exitrel = dsize, psize, exitrel
        self.code_end = anchor - 8


class Segment:
    def __init__(self, idx, name, data):
        self.idx, self.name, self.data = idx, name, data
        last = rd16(data, len(data) - 2)
        self.segnum = last & 0xFF
        self.nproc = last >> 8
        self.procs = []
        base = len(data) - 2                     # points at the last word
        for p in range(1, self.nproc + 1):
            ptr_pos = base - 2 * p
            val = rd16(data, ptr_pos)
            anchor = ptr_pos - val
            if not (8 <= anchor <= len(data) - 2):
                self.procs.append(Proc(p, anchor, -1, 0, 0, 0))
                continue
            dsize = rd16(data, anchor - 8)
            psize = rd16(data, anchor - 6)
            exitrel = rd16(data, anchor - 4)
            entry = (anchor - 2) - rd16(data, anchor - 2)
            self.procs.append(Proc(p, anchor, entry, dsize, psize, exitrel))

    def proc(self, n):
        for p in self.procs:
            if p.num == n:
                return p
        return None


def load_segments(path):
    data = open(path, "rb").read()
    segs = []
    for i in range(16):
        fb, ln = struct.unpack_from("<HH", data, i * 4)
        name = data[0x40 + i * 8:0x48 + i * 8].split(b"\x00")[0].decode("latin1").strip()
        if ln == 0:
            continue
        blob = data[fb * BLOCK: fb * BLOCK + ln]
        s = Segment(i, name, blob)
        segs.append(s)
        SEGNUM_NAME[s.segnum] = name
    return segs


def find_segment(segs, key):
    try:
        i = int(key)
        for s in segs:
            if s.idx == i:
                return s
    except ValueError:
        for s in segs:
            if s.name.upper() == key.upper():
                return s
    raise SystemExit(f"segment not found: {key}")


# --------------------------------------------------------------------------
def disasm_proc(seg, proc, out):
    global CUR_SEG
    CUR_SEG = seg.name
    d = seg.data
    anchor = proc.anchor
    ip = proc.entry
    end = proc.code_end
    if ip < 0 or ip >= end:
        out.append(f"; ---- {seg.name}:proc {proc.num}  (no code, anchor={anchor:#x}) ----")
        return

    # pass 1: walk instructions, collect jump targets and the lowest jump-table
    # slot referenced by a backward jump (the table sits just below the anchor).
    labels = set()
    jtab_lo = end
    scan = ip
    while scan < end:
        tgt, kind, nxt = decode_one(d, scan, anchor)
        if kind == "jump":
            fwd, tslot = tgt
            if fwd is not None:
                labels.add(fwd)
            if tslot is not None:
                jtab_lo = min(jtab_lo, tslot)
                labels.add(tslot - rd16(d, tslot))
        elif kind == "xjp" and tgt:
            labels.update(tgt)
        if nxt is None or nxt <= scan:
            break
        scan = nxt
    code_end = min(end, jtab_lo)

    nm = pname(seg.name, proc.num)
    out.append(f"; ---- {seg.name}:proc {proc.num}"
               f"{'  = ' + nm if nm else ''}  entry={ip:#x} code_end={code_end:#x} "
               f"data={proc.dsize} param={proc.psize} ----")

    max_label = max(labels) if labels else 0
    TERM = ("UJP", "RNP", "RBP", "HLT", "XJP")
    ip = proc.entry
    while ip < code_end:
        if ip in labels:
            out.append(f"L{ip:04x}:")
        text, _, nxt = decode_one(d, ip, anchor, pretty=True)
        raw = " ".join(f"{x:02x}" for x in d[ip:min(nxt, ip + 6)]) if nxt else ""
        out.append(f"  {ip:04x}  {raw:<17} {text}")
        if nxt is None or nxt <= ip:
            out.append(f"  {ip:04x}  ; <stop>")
            break
        mnem = text.split()[0]
        if mnem in TERM and nxt > max_label and nxt not in labels:
            code_end = nxt          # rest is alignment pad / jump table
            break
        ip = nxt
    if code_end < end:
        words = [rd16(d, o) for o in range(code_end | 1 if code_end & 1 else code_end, end, 2)]
        out.append(f"  ; jump table [{code_end:#x}..{end:#x}) "
                   + " ".join(f"L{(o)-rd16(d,o):04x}" for o in range(code_end + (code_end & 1), end, 2)))


def decode_one(d, ip, anchor, pretty=False):
    """Return (target_or_targets_or_None, kind, next_ip).  With pretty=True the
    first element is instead the formatted instruction string."""
    op = d[ip]
    p = ip + 1

    if op < 0x80:
        return (f"SLDC   {op}" if pretty else None), "sldc", p
    if 0xD8 <= op <= 0xE7:
        return (f"SLDL   {op - 0xD7}" if pretty else None), "-", p
    if 0xE8 <= op <= 0xF7:
        g = gname(op - 0xE7)
        return (f"SLDO   {op - 0xE7}" + (f"  ; {g}" if pretty and g else "")
                if pretty else None), "-", p
    if 0xF8 <= op <= 0xFF:
        return (f"SIND   {op - 0xF8}" if pretty else None), "-", p

    mnem, kind = OPS.get(op, (f"DB_{op:02x}", "-"))

    if kind == "-":
        return (f"{mnem}" if pretty else None), "-", p

    if kind == "big":
        v, p = rd_big(d, p)
        g = gname(v) if mnem in ("LDO", "SRO", "LAO") else None
        return (f"{mnem:<6} {v}" + (f"  ; {g}" if pretty and g else "")
                if pretty else None), "-", p

    if kind == "ub":
        v = d[p]
        if pretty and mnem in ("CLP", "CGP", "CIP", "CBP"):
            nm = pname(CUR_SEG, v)
            return f"{mnem:<6} {v}" + (f"  ; {nm}" if nm else ""), "call", p + 1
        return (f"{mnem:<6} {v}" if pretty else None), "-", p + 1

    if kind == "ubt":
        v = d[p]
        return (f"{mnem:<6} {CMP_TYPE.get(v, v)}" if pretty else None), "-", p + 1

    if kind == "ub_ub":
        return (f"{mnem:<6} {d[p]},{d[p+1]}" if pretty else None), "-", p + 2

    if kind == "word":
        v = rd16s(d, p)
        return (f"{mnem:<6} {v}" if pretty else None), "-", p + 2

    if kind == "csp":
        v = d[p]
        return (f"CSP    {CSP_NAMES.get(v, v)}" if pretty else None), "-", p + 1

    if kind == "ll_big":
        ll = d[p]
        v, p2 = rd_big(d, p + 1)
        return (f"{mnem:<6} {ll},{v}" if pretty else None), "-", p2

    if kind == "seg_big":
        sn = d[p]
        v, p2 = rd_big(d, p + 1)
        nm = SEGNUM_NAME.get(sn, sn)
        return (f"{mnem:<6} {nm},{v}" if pretty else None), "-", p2

    if kind == "str":
        ln = d[p]
        s = d[p + 1:p + 1 + ln]
        txt = s.decode("latin1").replace("\n", "\\n")
        return (f'{mnem:<6} {ln} "{txt}"' if pretty else None), "-", p + 1 + ln

    if kind == "ldc":
        cnt = d[p]
        q = p + 1
        if q & 1:
            q += 1
        words = [rd16(d, q + 2 * i) for i in range(cnt)]
        return (f"LDC    {cnt} {words}" if pretty else None), "-", q + 2 * cnt

    if kind == "cxp":
        sn, pn = d[p], d[p + 1]
        nm = SEGNUM_NAME.get(sn, f"seg{sn}")
        an = pname(nm, pn)
        return (f"CXP    {nm},{pn}" + (f"  ; {an}" if an else "") if pretty
                else None), "call", p + 2

    if kind == "jump":
        db = d[p]
        after = p + 1
        if db < 0x80:                        # forward, self-relative
            fwd, slot, tgt = after + db, None, after + db
        else:                               # backward: word@(anchor+signed) self-rel
            slot = anchor + (db - 256)
            tgt = slot - rd16(d, slot)
            fwd = None
        if pretty:
            return f"{mnem:<6} L{tgt:04x}", "jump", after
        return (fwd, slot), "jump", after

    if kind == "xjp":
        q = p + 1 if (p & 1) else p
        lo = rd16s(d, q)
        hi = rd16s(d, q + 2)
        ncase = hi - lo + 1
        tbl = q + 4
        tgts = []
        for i in range(max(ncase, 0)):
            slot = tbl + 2 * i
            tgts.append(slot - rd16(d, slot))
        nxt = tbl + 2 * max(ncase, 0)
        if pretty:
            return (f"XJP    {lo}..{hi} -> " +
                    ", ".join(f"L{t:04x}" for t in tgts)), "xjp", nxt
        return tgts, "xjp", nxt

    return (f"{mnem}" if pretty else None), "-", p


def rd_big(d, p):
    b0 = d[p]
    if b0 < 0x80:
        return b0, p + 1
    return ((b0 & 0x7F) << 8) | d[p + 1], p + 2


# --------------------------------------------------------------------------
def cmd_segments(a):
    segs = load_segments(a[0])
    print(f"  {'idx':>3} {'name':<10} {'segnum':>6} {'procs':>6} {'bytes':>7}")
    for s in segs:
        print(f"  {s.idx:>3} {s.name:<10} {s.segnum:>6} {s.nproc:>6} {len(s.data):>7}")


def cmd_procs(a):
    segs = load_segments(a[0])
    s = find_segment(segs, a[1])
    print(f"segment {s.name} (segnum {s.segnum}), {s.nproc} procs\n")
    print(f"  {'proc':>4} {'entry':>6} {'end':>6} {'anchor':>6} {'data':>6} {'param':>5}")
    for p in sorted(s.procs, key=lambda x: x.entry):
        print(f"  {p.num:>4} {p.entry:>6} {p.code_end:>6} {p.anchor:>6} {p.dsize:>6} {p.psize:>5}")


def cmd_dis(a):
    segs = load_segments(a[0])
    s = find_segment(segs, a[1])
    out = []
    procs = s.procs if len(a) < 3 else [s.proc(int(a[2]))]
    for p in sorted(procs, key=lambda x: x.entry):
        disasm_proc(s, p, out)
        out.append("")
    print("\n".join(out))


def cmd_dis_all(a):
    segs = load_segments(a[0])
    outdir = a[1]
    os.makedirs(outdir, exist_ok=True)
    for s in segs:
        out = [f"; segment {s.name}  segnum={s.segnum}  procs={s.nproc}\n"]
        for p in sorted(s.procs, key=lambda x: x.entry):
            disasm_proc(s, p, out)
            out.append("")
        with open(os.path.join(outdir, f"{s.idx:02d}_{s.name}.pd"), "w") as fh:
            fh.write("\n".join(out))
    print(f"wrote {len(segs)} listings to {outdir}/")


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        sys.exit(2)
    load_procmap()
    load_globals()
    {"segments": cmd_segments, "procs": cmd_procs, "dis": cmd_dis,
     "dis-all": cmd_dis_all}[argv[0]](argv[1:])


if __name__ == "__main__":
    main(sys.argv[1:])
