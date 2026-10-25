#!/usr/bin/env python3
"""
A small decompiler for the DOS Wizardry p-code: symbolic evaluation of the
eval stack into expressions, goto-style control flow (labels at jump targets,
`if (!c) goto L`), names from docs/globals.tsv + known WIZARDRY UI procs.

    python tools/pcode_decomp.py SEG PROC [PROC ...]
    python tools/pcode_decomp.py SEG all

Not a compiler-grade decompiler -- goal is "readable enough to port".
Conventions printed:  Lk = local word k, Pk = parameter word k (params are the
first psize/2 words), Gk = global word k (name from globals.tsv when known),
Uk.j = word j of the frame k static links up; &x = address; *(p+k) = word load;
p.b[i] = byte; bits(p,w,s) = packed-field load; string literals as 'text'.
"""
import os, sys, struct
sys.path.insert(0, os.path.dirname(__file__))
import pcode_dis as pd

ROOT = os.path.join(os.path.dirname(__file__), '..')
PASCAL = os.path.join(ROOT, 'extracted', 'wiz1', 'SYSTEM.PASCAL')
pd.load_procmap(os.path.join(ROOT, 'docs', 'procmap.tsv'))
pd.load_globals(os.path.join(ROOT, 'docs', 'globals.tsv'))
SEGS = pd.load_segments(PASCAL)
BYNAME = {s.name: s for s in SEGS}
BYNUM = {s.segnum: s for s in SEGS}

# known WIZARDRY (seg 1) procs: name + parameter names in *push order*
KNOWN = {
    19: ('WinOpen', 'x y w h prio refresh a b'.split()),
    20: ('WinClose', 'pwin refresh'.split()),
    21: ('WinSetPrio', 'win prio refresh'.split()),
    22: ('GotoXY', 'win x y'.split()),
    23: ('PutNum', 'win n width'.split()),
    24: ('Newline', 'win'.split()),
    25: ('WinClear', 'win refresh'.split()),
    26: ('WinClearRect', 'win x y w h'.split()),
    27: ('WinHide', 'win refresh'.split()),
    28: ('WinShow', 'win refresh'.split()),
    13: ('PutCentered', 'win s'.split()),
    29: ('GetLine', 'win s maxlen'.split()),
    30: ('Menu', 'win s a b'.split()),
    35: ('PutC', 'win ch'.split()),
    36: ('PutS', 'win s'.split()),
    37: ('Refresh', []),
    38: ('GetStr', 'n pstr'.split()),
    39: ('PutStr', 'win n'.split()),
    40: ('PutStrLn', 'win n'.split()),
    41: ('MenuStr', 'n win'.split()),
    42: ('VisLen', 's'.split()),
    43: ('BoxMsg', 'n x y w'.split()),
    11: ('GetKey', []),
    12: ('WaitCR', []),
    33: ('KeyReady', 'a b'.split()),
    34: ('Random', 'a b'.split()),
    59: ('FindFile', 'unit name a b'.split()),
    67: ('RefreshIf', 'flag'.split()),
    68: ('SortWins', []),
    18: ('WinAlloc', 'size a'.split()),
    17: ('WinFree', 'win'.split()),
    16: ('WinGC', []),
    78: ('GetLineBackspace', []),
    77: ('ClampXY', 'win px py'.split()),
    45: ('StrAppend', 'dst src'.split()),
    46: ('StrAppend2', 'dst s1 s2'.split()),
    47: ('StrAppend3', 'dst s1 s2 s3'.split()),
    48: ('StrCopy', 'dst src pos len'.split()),
    49: ('StrPos', 'pat s'.split()),
    50: ('StrPos2', 'pat s'.split()),
    10: ('PutPadded11', 'win s'.split()),
}


class Dec:
    def __init__(self, seg, proc):
        self.seg, self.proc = seg, proc
        self.d = seg.data
        self.out = []
        self.st = []
        self.labels = set()
        self.np = proc.psize // 2
        self.is_func = False
        self.rn = self.rnp_count(seg, proc)

    # -- helpers ----------------------------------------------------------
    def callee(self, segname, n):
        s = BYNAME.get(segname) or BYNUM.get(segname)
        p = s.proc(n) if s else None
        return s, p

    def rnp_count(self, seg, proc):
        """the RNP operand of a proc (function result words)"""
        if proc is None or proc.entry < 0: return 0
        ip, end = proc.entry, proc.code_end
        last = 0
        while ip < end:
            try:
                txt, kind, nxt = pd.decode_one(seg.data, ip, proc.anchor, pretty=True)
            except Exception:
                break
            if txt.startswith('RNP'):
                last = int(txt.split()[1])
            if nxt is None or nxt <= ip: break
            ip = nxt
        return last

    def L(self, k):
        if k > self.np:
            return f'L{k - self.np}'          # locals numbered from 1 after the params
        j = self.np - k                      # push index (0 = first pushed)
        rn = self.rn
        if k <= rn:
            return 'result' if rn == 1 else f'result{k}'
        names = None
        if self.seg.name == 'WIZARDRY' and self.proc.num in KNOWN:
            names = KNOWN[self.proc.num][1]
        if names and j < len(names):
            return names[j]
        return f'a{j + 1}'

    def G(self, k):
        n = pd.gname(k)
        return n if n else f'G{k}'

    def pop(self):
        return self.st.pop() if self.st else '?'

    def emit(self, s):
        self.out.append(s)

    def push(self, s):
        self.st.append(s)

    # -- main ---------------------------------------------------------------
    def run(self):
        d, p = self.d, self.proc
        ip, end = p.entry, p.code_end
        anchor = p.anchor
        # labels: collect jump targets
        scan = ip
        while scan < end:
            txt, kind, nxt = pd.decode_one(d, scan, anchor, pretty=True)
            if kind == 'jump':
                self.labels.add(int(txt.split('L')[-1], 16))
            elif kind == 'xjp':
                for t in txt.split('->')[1].split(','):
                    self.labels.add(int(t.strip()[1:], 16))
            if nxt is None or nxt <= scan: break
            scan = nxt
        name = pd.pname(self.seg.name, p.num) or ''
        hdr = f'{self.seg.name}:{p.num} {name}  params={p.psize//2} locals={p.dsize//2}'
        self.emit(f'// ---- {hdr}')
        rn = self.rnp_count(self.seg, p)
        while ip < end:
            if ip in self.labels:
                self.flush_stack()
                self.emit(f'L{ip:04x}:')
            txt, kind, nxt = pd.decode_one(d, ip, anchor, pretty=True)
            self.step(ip, d[ip], txt, nxt)
            if nxt is None or nxt <= ip: break
            ip = nxt
        self.flush_stack()
        return '\n'.join(self.out)

    def flush_stack(self):
        if self.st:
            self.emit(f'   // stack: {self.st}')
            self.st = []

    def step(self, ip, op, txt, nxt):
        d = self.d
        st = self.st
        parts = txt.split(None, 1)
        m = parts[0]
        arg = parts[1] if len(parts) > 1 else ''
        P = self.push
        if m == 'SLDC': P(arg)
        elif m == 'SLDL': P(self.L(int(arg)))
        elif m == 'LDL': P(self.L(int(arg)))
        elif m == 'SLDO' or m == 'LDO': P(self.G(int(arg.split()[0])))
        elif m == 'LDCN': P('NIL')
        elif m == 'LDCI': P(arg)
        elif m == 'LLA': P(f'&{self.L(int(arg))}')
        elif m == 'LAO': P(f'&{self.G(int(arg.split()[0]))}')
        elif m == 'LOD':
            ll, k = arg.split(',')
            P(f'U{ll}.{k}')
        elif m == 'LDA':
            ll, k = arg.split(',')
            P(f'&U{ll}.{k}')
        elif m == 'LSA':
            lit = arg.split('"', 1)[1].rsplit('"', 1)[0] if '"' in arg else ''
            P(repr(lit))
        elif m == 'SIND': P(f'*({self.pop()}+{int(arg)})' if int(arg) else f'*{self.pop()}')
        elif m == 'IND':
            k = int(arg); a = self.pop(); P(f'*({a}+{k})' if k else f'*{a}')
        elif m == 'INC':
            k = int(arg); a = self.pop(); P(f'({a}+{k}w)')
        elif m == 'IXA':
            k = int(arg); i = self.pop(); a = self.pop(); P(f'{a}[{i}]' if k == 1 else f'{a}[{i}]*{k}w')
        elif m == 'IXP':
            i = self.pop(); a = self.pop(); P(f'{a}.pk[{i}]')   # then LDP
            self.pk = arg
        elif m == 'LDP':
            s = self.pop(); w = self.pop(); a = self.pop(); P(f'bits({a},{w},{s})')
        elif m == 'STP':
            v = self.pop(); s = self.pop(); w = self.pop(); a = self.pop()
            self.emit(f'   bits({a},{w},{s}) := {v}')
        elif m == 'LDB':
            i = self.pop(); a = self.pop(); P(f'{a}.b[{i}]')
        elif m == 'STB':
            v = self.pop(); i = self.pop(); a = self.pop(); self.emit(f'   {a}.b[{i}] := {v}')
        elif m in ('ADI', 'SBI', 'MPI', 'DVI', 'MODI', 'LAND', 'LOR'):
            b = self.pop(); a = self.pop()
            sym = {'ADI': '+', 'SBI': '-', 'MPI': '*', 'DVI': 'div', 'MODI': 'mod', 'LAND': 'and', 'LOR': 'or'}[m]
            P(f'({a} {sym} {b})')
        elif m == 'NGI': P(f'-{self.pop()}')
        elif m == 'ABI': P(f'abs({self.pop()})')
        elif m == 'LNOT': P(f'not {self.pop()}')
        elif m in ('EQUI', 'NEQI', 'LEQI', 'LESI', 'GEQI', 'GTRI'):
            b = self.pop(); a = self.pop()
            sym = {'EQUI': '=', 'NEQI': '<>', 'LEQI': '<=', 'LESI': '<', 'GEQI': '>=', 'GTRI': '>'}[m]
            P(f'({a} {sym} {b})')
        elif m in ('EQU', 'NEQ', 'LEQ', 'LES', 'GEQ', 'GTR'):
            sym = {'EQU': '=', 'NEQ': '<>', 'LEQ': '<=', 'LES': '<', 'GEQ': '>=', 'GTR': '>'}[m]
            b = self.pop(); a = self.pop()
            if arg.startswith(('BYTE', 'WORD')): P(f'cmp[{arg}]({a} {sym} {b})')
            else: P(f'({a} {sym} {b})')
        elif m == 'STL': self.emit(f'   {self.L(int(arg))} := {self.pop()}')
        elif m == 'STR':
            ll, k = arg.split(','); self.emit(f'   U{ll}.{k} := {self.pop()}')
        elif m == 'SRO': self.emit(f'   {self.G(int(arg.split()[0]))} := {self.pop()}')
        elif m == 'STO':
            v = self.pop(); a = self.pop(); self.emit(f'   *{a} := {v}')
        elif m == 'SAS':
            s = self.pop(); a = self.pop(); self.emit(f'   {a} := {s}   // string assign (max {arg})')
        elif m == 'IXS':
            i = self.pop(); a = self.pop(); P(f'{a}.c[{i}]')    # followed by LDB 0?
        elif m == 'MOV':
            s = self.pop(); dd = self.pop(); self.emit(f'   move {arg}w {dd} <- {s}')
        elif m == 'FJP':
            c = self.pop(); self.emit(f'   if not {c} goto {arg}')
        elif m == 'UJP':
            self.flush_stack(); self.emit(f'   goto {arg}')
        elif m == 'XJP':
            v = self.pop(); self.emit(f'   switch {v}: {arg}')
        elif m == 'CSP':
            self.csp(arg)
        elif m in ('CXP', 'CLP', 'CIP', 'CGP'):
            self.call(m, arg)
        elif m == 'RNP':
            n = int(arg)
            self.flush_stack()
            self.emit(f'   return' + (f' {n} word(s) from L1..' if n else ''))
        elif m in ('NOP',): pass
        else:
            self.emit(f'   ?? {txt}')

    def csp(self, name):
        st = self.st
        if name in ('UNITREAD', 'UNITWRITE'):
            w = self.pop(); z = self.pop(); y = self.pop(); x = self.pop(); b = self.pop(); u = self.pop()
            self.emit(f'   {name}(unit={u}, buf={b}+{x}, sel/len={y}, z/blk={z}, w={w})')
        elif name == 'MOVELEFT':
            c = self.pop(); do = self.pop(); dp = self.pop(); so = self.pop(); sp = self.pop()
            self.emit(f'   MOVELEFT({sp}+{so} -> {dp}+{do}, {c})')
        elif name == 'FILLCHAR':
            ch = self.pop(); n = self.pop(); o = self.pop(); p = self.pop()
            self.emit(f'   FILLCHAR({p}+{o}, {n}, {ch})')
        elif name == 'EXIT':
            p = self.pop(); s = self.pop(); self.emit(f'   EXIT({s}:{p})')
        elif name == '34': self.push('IORESULT')
        elif name == '40': self.push('MEMAVAIL')
        elif name == '38': self.emit(f'   FLUSHKEYS({self.pop()})')
        elif name in ('MARK', 'RELEASE'): self.emit(f'   {name}({self.pop()})')
        elif name == 'NEW': a = self.pop(); b = self.pop(); self.emit(f'   NEW({b}, {a} words)')
        else: self.emit(f'   CSP {name}')

    def call(self, m, arg):
        d = self.d
        if m == 'CXP':
            segn, pn = arg.split()[0].split(',')
            seg, proc = self.callee(segn, int(pn))
            pn = int(pn)
            nm = None
            if segn == 'WIZARDRY' and pn in KNOWN: nm = KNOWN[pn][0]
            else: nm = pd.pname(segn, pn) or f'{segn}.p{pn}'
            label = f'{segn}:{pn}' + (f' {nm}' if nm and nm != f'{segn}.p{pn}' else '')
        else:
            pn = int(arg.split()[0])
            seg, proc = self.seg, self.seg.proc(pn)
            nm = None
            if self.seg.name == 'WIZARDRY' and pn in KNOWN: nm = KNOWN[pn][0]
            else: nm = pd.pname(self.seg.name, pn)
            label = f'{pn}' + (f' {nm}' if nm else '')
        n = proc.psize // 2 if proc else 0
        rn = self.rnp_count(seg, proc) if proc else 0
        args = [self.pop() for _ in range(n)]
        args.reverse()                      # push order
        if rn:
            args = args[:n - rn]            # drop result placeholders (last pushed)
        known = KNOWN.get(pn) if (seg and seg.name == 'WIZARDRY') else None
        if known and len(known[1]) == len(args):
            args = [f'{k}={v}' for k, v in zip(known[1], args)]
        call = f'{label}({", ".join(args)})'
        if rn:
            self.push(call)
        else:
            self.emit(f'   {call}')


def main(a):
    if len(a) < 2:
        print(__doc__); return
    seg = BYNAME.get(a[0].upper())
    if seg is None:
        seg = BYNUM[int(a[0])]
    procs = seg.procs if a[1] == 'all' else [seg.proc(int(x)) for x in a[1:]]
    for p in sorted(procs, key=lambda q: q.entry):
        print(Dec(seg, p).run())
        print()


if __name__ == '__main__':
    main(sys.argv[1:])
