"""
renames_lib.py - shared, IDA-independent code for the rename tables in renames/.

A rename table is a TSV, one per disassembly (renames/<game>/<stem>.tsv, where
<game>/<stem> is the IDB under disasm/).  Columns (tab separated, '#' lines are
comments, blank lines ignored):

    ea       linear IDA address, hex (0x1C7D1).  These 16-bit IDBs are loaded at
             1000:0, so ea = seg*16 + off and 1000:C7D1 == 0x1C7D1.
    segoff   seg:off form of ea (informational; checked/filled by tools/renames.py)
    kind     func | data | label
    old      name in the committed .asm export (sub_1C7D1, byte_1F727, ...) or empty
    new      the name to give it
    conf     verified | likely | guess   (see renames/README.md)
    size     bytes, for data (optional)
    sig      expected bytes at ea as hex, '??' = wildcard (optional, but strongly
             recommended: lets the apply script refuse to rename the wrong place)
    comment  free text, applied as a (function) comment when none exists yet
"""
import os
import re
import struct
from dataclasses import dataclass

CONF_ORDER = {'guess': 0, 'likely': 1, 'verified': 2}
KINDS = ('func', 'data', 'label')
NAME_RE = re.compile(r'^[A-Za-z_@?$.][A-Za-z0-9_@?$.]*$')
# names IDA generates itself (anything else in an IDB is somebody's rename)
AUTO_NAME_RE = re.compile(
    r'^(sub|loc|locret|byte|word|dword|qword|unk|off|asc|stru|nullsub|flt|dbl|seg|'
    r'def|algn|stk|var|arg)_[0-9A-Fa-f]+$|^(start|j_.*)$')
HEADER = ['ea', 'segoff', 'kind', 'old', 'new', 'conf', 'size', 'sig', 'comment']

LOAD_BASE = 0x10000  # linear address of image byte 0 in every IDB of this project


@dataclass
class Row:
    ea: int
    segoff: str
    kind: str
    old: str
    new: str
    conf: str
    size: int
    sig: str
    comment: str
    line: int = 0

    def fmt(self):
        return '\t'.join([
            '0x%X' % self.ea, self.segoff, self.kind, self.old, self.new, self.conf,
            str(self.size) if self.size else '', self.sig, self.comment])


# ------------------------------------------------------------------ parsing
def parse_tsv(path):
    rows, errors = [], []
    with open(path, encoding='utf-8') as f:
        for n, line in enumerate(f, 1):
            line = line.rstrip('\n')
            if not line.strip() or line.lstrip().startswith('#'):
                continue
            cols = line.split('\t')
            cols += [''] * (len(HEADER) - len(cols))
            if len(cols) > len(HEADER):
                errors.append('%s:%d: too many columns' % (path, n))
                continue
            ea_s, segoff, kind, old, new, conf, size, sig, comment = cols
            try:
                ea = int(ea_s, 16)
            except ValueError:
                errors.append('%s:%d: bad ea %r' % (path, n, ea_s))
                continue
            if kind not in KINDS:
                errors.append('%s:%d: bad kind %r' % (path, n, kind))
            if conf not in CONF_ORDER:
                errors.append('%s:%d: bad conf %r' % (path, n, conf))
            if not NAME_RE.match(new):
                errors.append('%s:%d: bad new name %r' % (path, n, new))
            if sig and not re.fullmatch(r'([0-9A-Fa-f]{2}|\?\?)( ?([0-9A-Fa-f]{2}|\?\?))*', sig):
                errors.append('%s:%d: bad sig %r' % (path, n, sig))
            rows.append(Row(ea, segoff, kind, old, new, conf, int(size) if size.isdigit() else 0,
                            sig.upper(), comment, n))
    return rows, errors


def write_tsv(path, rows, header_comment=''):
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        if header_comment:
            for l in header_comment.rstrip('\n').split('\n'):
                f.write('# ' + l + '\n')
        f.write('# ' + '\t'.join(HEADER) + '\n')
        for r in sorted(rows, key=lambda r: r.ea):
            f.write(r.fmt() + '\n')


def sig_bytes(sig):
    """'55 8B ?? 1E' -> [0x55, 0x8B, None, 0x1E]"""
    sig = sig.replace(' ', '')
    return [None if sig[i:i + 2] == '??' else int(sig[i:i + 2], 16) for i in range(0, len(sig), 2)]


def sig_matches(data, sig):
    pat = sig_bytes(sig)
    if len(data) < len(pat):
        return False
    return all(p is None or p == b for p, b in zip(pat, data))


# ------------------------------------------------------------ images / segs
class Image:
    """The unpacked executable as IDA saw it (MZ header stripped, loaded at
    linear 0x10000), plus the segment table from the committed .idc export."""

    def __init__(self, exe_path, idc_path=None):
        with open(exe_path, 'rb') as fh:
            d = fh.read()
        if d[:2] == b'MZ':
            hdrpar = struct.unpack('<H', d[8:10])[0]
            d = d[hdrpar * 16:]
        self.data = d
        self.segs = []  # (start, end, para)
        if idc_path and os.path.exists(idc_path):
            for m in re.finditer(r'add_segm_ex\((0[Xx][0-9A-Fa-f]+),(0[Xx][0-9A-Fa-f]+),(0[Xx][0-9A-Fa-f]+)',
                                 open(idc_path, encoding='latin1').read()):
                self.segs.append(tuple(int(x, 16) for x in m.groups()))

    def bytes_at(self, ea, n):
        o = ea - LOAD_BASE
        if o < 0 or o >= len(self.data):
            return b''
        return self.data[o:o + n]

    def seg_of(self, ea):
        for s, e, p in self.segs:
            if s <= ea < e:
                return p
        return None

    def segoff(self, ea):
        p = self.seg_of(ea)
        if p is None:
            return ''
        return '%04X:%04X' % (p, ea - p * 16)


def default_sig(img, ea, n=16):
    b = img.bytes_at(ea, n)
    return ' '.join('%02X' % x for x in b)


# ------------------------------------------------------- cross-build search
def mask_relocatable(b):
    """Wildcard operands that change when code moves between two builds
    (rel16 call/jmp, far pointers, mov r16,imm16)."""
    out, i = [], 0
    while i < len(b):
        op = b[i]
        if op in (0xE8, 0xE9) and i + 2 < len(b):
            out += [op, None, None]; i += 3
        elif op in (0x9A, 0xEA) and i + 4 < len(b):
            out += [op] + [None] * 4; i += 5
        elif 0xB8 <= op <= 0xBF and i + 2 < len(b):
            out += [op, None, None]; i += 3
        else:
            out.append(op); i += 1
    return out


def masked_pattern(img, ea, nbytes):
    """Instruction-aware pattern of the code at ea: every 16-bit displacement /
    immediate / branch operand is wildcarded (those change when code moves),
    opcode and ModRM bytes are kept.  Needs capstone; falls back to a crude
    byte-level heuristic without it."""
    raw = img.bytes_at(ea, nbytes + 16)
    try:
        import capstone
    except ImportError:
        return mask_relocatable(raw[:nbytes])
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    md.detail = True
    pat = []
    for ins in md.disasm(raw, 0):
        b = list(raw[ins.address:ins.address + ins.size])
        e = ins.encoding
        for off, size in ((e.disp_offset, e.disp_size), (e.imm_offset, e.imm_size)):
            if size >= 2 and off:
                for k in range(off, off + size):
                    b[k] = None
        pat += b
        if len(pat) >= nbytes:
            break
    return pat


def _search(dst, pat):
    rx = b''.join(re.escape(bytes([p])) if p is not None else b'.' for p in pat)
    return [m.start() for m in re.finditer(rx, dst, re.S)]


def find_equivalent(src_img, src_ea, dst_img, hint_delta=None):
    """Locate the routine at src_ea of one build inside another build.
    Returns (dst_ea, how, score) or (None, reason, 0).
    1. unique instruction-aware masked match over 16..64 bytes (globally, or
       - when several remain - unique within 0x800 of src_ea+hint_delta)
    2. best fuzzy byte match near the hint (>= 26/32 equal bytes, clear margin)."""
    dst = dst_img.data
    for L in (16, 24, 32, 48, 64):
        pat = masked_pattern(src_img, src_ea, L)
        hits = _search(dst, pat)
        if len(hits) == 1:
            return hits[0] + LOAD_BASE, 'sig%d' % L, 1.0
        if hits and hint_delta is not None:
            near = [h for h in hits if abs(h + LOAD_BASE - (src_ea + hint_delta)) <= 0x800]
            if len(near) == 1:
                return near[0] + LOAD_BASE, 'sig%d-near' % L, 1.0
    if hint_delta is None:
        return None, 'no unique signature match and no neighbour delta', 0
    src = src_img.bytes_at(src_ea, 48)
    centre = src_ea + hint_delta - LOAD_BASE
    best = []
    for o in range(max(0, centre - 0x400), min(len(dst) - 32, centre + 0x400)):
        eq = sum(1 for a, b in zip(src[:32], dst[o:o + 32]) if a == b)
        best.append((eq, o))
    best.sort(reverse=True)
    if best and best[0][0] >= 26 and (len(best) < 2 or best[0][0] > best[1][0]):
        return best[0][1] + LOAD_BASE, 'fuzzy', best[0][0] / 32.0
    return None, 'no match (best fuzzy %d/32)' % (best[0][0] if best else 0), 0
