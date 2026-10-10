import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ASM = os.path.join(ROOT, "Deponia_Linux.asm")
FIELDS = os.path.join(ROOT, "src", "deponia1", "vstables", "fieldIds.h")

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from uvpaths import ins, labels, inv, reach, entry

# field names by id
names = {}
for l in open(FIELDS, encoding='utf-8'):
    m = re.match(r'\s*(k\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)', l)
    if m:
        names.setdefault(int(m.group(2), 0), []).append(m.group(1))

# re-read with comments
lines = open(ASM, encoding='utf-8', errors='replace').read().split('\n')[1508676:1523438]
cins = []
for l in lines:
    if not l.strip() or l.strip().startswith(';'):
        continue
    m = re.match(r'^(loc_[0-9A-F]+|[A-Za-z_]\w*):', l)
    if m:
        rest = l[m.end():]
        code = rest.split(';')[0].strip()
        cm = rest.split(';', 1)[1].strip() if ';' in rest else ''
        if code:
            cins.append((cm, code))
        continue
    code = l.split(';')[0].strip()
    cm = l.split(';', 1)[1].strip() if ';' in l else ''
    if code:
        cins.append((cm, code))
assert len(cins) == len(ins), (len(cins), len(ins))

NOISE = re.compile(r'^(sub\s+rbp,\s*18h|cmp\s+rbp,\s*offset|mov\s+rbp,\s*\[rsp\+0C78h\+var_\w+\]|lea\s+rdi,\s*\[rbp\+10h\]|test\s+eax,\s*eax|jg\s+|mov\s+rdi,\s*\[rsp|jnz\s+loc_93A[0-9A-F]+|test\s+rdi,\s*rdi|lea\s+r\w+,\s*\[rsp|mov\s+r\w+,\s*\[rsp|lea\s+r\w+,\s*\[rsp|mov\s+r(bp|bx|1[2-5]),\s*(rax|r\w+)$|mov\s+r(di|si),\s*r(bx|bp|1[2-5])$|add\s+rbp,\s*8$|add\s+r13,\s*8$|add\s+r12,\s*8$|nop|xchg|db\s|align|mov\s+\[rsp)')


def dump(V, only_diff=True):
    r = reach(V)
    nxt = reach(V + 1) if V < 0xBA else set()
    sel = sorted(r - nxt) if only_diff else sorted(r)
    out = []
    last = None
    for p in sel:
        if last is not None and p != last + 1:
            out.append('   ----')
        last = p
        if p in inv:
            out.append('%s:' % inv[p][0])
        cm, code = cins[p]
        if (NOISE.match(code) and not code.startswith('call') and not code.startswith('j')) or 'exchange_and_add' in code or '_M_destroy' in code or '~TVisObjRef' in cm or ('basic_string' in cm and 'wchar_t const*' not in cm):
            continue
        ann = ''
        m = re.match(r'mov\s+(\w+),\s*([0-9A-F]+)h$', code)
        if m and int(m.group(2), 16) in names:
            ann = '   ;' + '/'.join(names[int(m.group(2), 16)][:3])
        m = re.match(r'mov\s+edx,\s*([0-9A-F]+)h$', code)
        out.append('  %s%s%s' % (code, ann, ('   ; ' + cm) if cm and not ann else ''))
    return '\n'.join(out)


if __name__ == '__main__':
    V = int(sys.argv[1], 16)
    print(dump(V))


def rawdump(V, only_diff=True):
    r = reach(V)
    nxt = reach(V + 1) if V < 0xBA else set()
    sel = sorted(r - nxt) if only_diff else sorted(r)
    # split into basic chunks at gaps; drop chunks that are refcount cleanup
    chunks = []
    for p in sel:
        if chunks and p == chunks[-1][-1] + 1:
            chunks[-1].append(p)
        else:
            chunks.append([p])
    out = []
    for ch in chunks:
        if any('exchange_and_add' in cins[p][1] or 'exchange_and_add' in cins[p][0] for p in ch):
            continue
        i = 0
        lines2 = []
        while i < len(ch):
            p = ch[i]
            cm, code = cins[p]
            if p in inv and lines2 and not lines2[-1].endswith(':'):
                pass
            if p in inv:
                lines2.append('%s:' % inv[p][0])
            # refcount check triple
            if re.match(r'sub\s+rbp,\s*18h', code) or re.match(r'cmp\s+rbp,\s*offset', code):
                i += 1
                continue
            if re.match(r'mov\s+rbp,\s*\[rsp\+0C78h\+var_\w+\]', code) and i + 1 < len(ch) and re.match(r'sub\s+rbp,\s*18h', cins[ch[i + 1]][1]):
                i += 1
                continue
            if re.match(r'jnz\s+loc_93[A-C]', code) and i > 0 and re.match(r'cmp\s+rbp,\s*offset', cins[ch[i - 1]][1]):
                i += 1
                continue
            if '~TVisObjRef' in cm or '_M_destroy' in cm or code.startswith('nop') or code.startswith('xchg'):
                if '~TVisObjRef' in cm:
                    lines2.append('    ; ~TVisObjRef')
                i += 1
                continue
            ann = ''
            m = re.match(r'mov\s+(\w+),\s*([0-9A-F]+)h$', code)
            if m and int(m.group(2), 16) in names:
                ann = '   ;' + '/'.join(names[int(m.group(2), 16)][:3])
            lines2.append('  %s%s%s' % (code, ann, ('   ; ' + cm[:60]) if cm and not ann else ''))
            i += 1
        out.append('\n'.join(lines2))
    return '\n   ----\n'.join(out)
