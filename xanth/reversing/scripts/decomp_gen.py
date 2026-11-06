"""Driver: evaluate a function's blocks, resolve joins, structure the control flow, render C++ (decomp.py)."""
import os
import re
import sys

from decomp import (L, STRIP, K, is_k, simp, negate_cond, Unsupported, walk, JCC)
from decomp_eval import build_cfg, learn_signatures, CALLSIG, Block
from decomp_exec import Evaluator, State, UNDEF


class Ctx:
    """the loaded assembly listing"""
    def __init__(self):
        self.insns, self.labels, self.procs, self.tables, self.locs = L.load_asm()
        learn_signatures(self.insns)
        self.ret_used = self.compute_ret_used()
        self.strings = self.load_strings()
        self.arrays, self.kinds = self.load_data()
        try:
            self.image = open(os.path.join(os.path.dirname(L.ASM), "game_out", "image.bin"), "rb").read()
        except OSError:
            self.image = b""

    DSEG = 0x84FC0

    def dseg_string(self, off):
        """the NUL-terminated text at dseg:off of the original's memory image (None if there is none)"""
        a = self.DSEG + off
        if not self.image or a >= len(self.image):
            return None
        end = self.image.find(bytes(1), a)
        if end < 0 or end - a > 200:
            return None
        t = self.image[a:end]
        if not t or any(c < 32 or c > 126 for c in t):
            return None
        return t.decode("ascii")

    def load_data(self):
        """data symbols of the listing: arrays (`name dw 20h dup(0)`) -> (element size, count); scalars -> db / dw / dd"""
        arrays, kinds = {}, {}
        size = {"db": 1, "dw": 2, "dd": 4}
        for line in open(L.ASM, encoding="utf-8", errors="replace"):
            m = re.match(r"^(\w+)\s+(db|dw|dd)\s+([0-9A-Fa-f]+h?)\s+dup\(", line)
            if m:
                n = m.group(3)
                cnt = int(n[:-1], 16) if n.lower().endswith("h") else int(n)
                arrays[m.group(1)] = (size[m.group(2)], cnt)
                continue
            m = re.match(r"^(\w+)\s+(db|dw|dd)\s", line)
            if m:
                kinds[m.group(1)] = m.group(2)
        return arrays, kinds

    def load_strings(self):
        """data symbols defined as text: `aName db 'text',0`"""
        out = {}
        for line in open(L.ASM, encoding="utf-8", errors="replace"):
            m = re.match(r"^(\w+)\s+db\s+(.*)$", line)
            if not m or "'" not in m.group(2):
                continue
            body = m.group(2).split(";")[0]
            txt, ok = "", True
            for part in re.findall(r"'([^']*)'|(\d[0-9A-Fa-f]*h?)", body):
                if part[0] or (not part[1]):
                    txt += part[0]
                else:
                    try:
                        v = int(part[1][:-1], 16) if part[1].lower().endswith("h") else int(part[1])
                    except ValueError:
                        continue
                    if v == 0:
                        break
                    txt += chr(v) if 32 <= v < 127 else "\\x%02X" % v
            out[m.group(1)] = txt
        return out

    def compute_ret_used(self):
        used = {}
        ins = self.insns
        for i, x in enumerate(ins[:-2]):
            if x.mn != "call" or not x.ops:
                continue
            tgt = STRIP.sub("", x.ops[0].strip())
            j = i + 1
            if ins[j].mn == "add" and ins[j].ops and ins[j].ops[0].strip() == "sp":
                j += 1
            n = ins[j]
            txt = " ".join(n.ops)
            r = 0
            if n.mn in ("xor", "sub") and n.ops and n.ops[0].strip() == n.ops[1].strip():
                r = 0
            elif n.mn == "mov" and re.match(r"^(ax|al)$", n.ops[0].strip()):
                r = 0
            elif re.search(r"\b(ax|al|ah)\b", txt) or n.mn in ("cwd", "cbw"):
                r = 1
            if re.search(r"\bdx\b", txt) and not (n.mn == "mov" and n.ops[0].strip() == "dx"):
                r = 2
            used[tgt] = max(used.get(tgt, 0), r)
        return used


# --------------------------------------------------------------------------------------------------------------------------------- evaluation of a function
def retonly(ctx, b):
    """a block that only returns (pops, mov sp,bp, retf)"""
    if ctx.insns[b.insns[-1]].mn not in ("retf", "retn", "ret"):
        return False
    for i in b.insns:
        x = ctx.insns[i]
        if not (x.mn in ("pop", "retf", "retn", "ret", "nop") or (x.mn == "mov" and x.ops[0].strip() == "sp")):
            return False
    return True


def join_state(f, b, exit_state):
    """entry state of a block with several predecessors: what all predecessors agree on, placeholders for the rest"""
    st = State()
    preds = [p for p in b.pred if p in exit_state]
    if not preds or len(preds) != len(set(b.pred)):
        return st
    ex = [exit_state[p] for p in preds]
    for r in set().union(*[set(e.regs) for e in ex]):
        vals = [e.regs.get(r) for e in ex]
        if all(v is not None and v == vals[0] for v in vals):
            st.regs[r] = vals[0]
    n = len(ex[0].stack)
    if all(len(e.stack) == n for e in ex):
        for k in range(n):
            vals = [e.stack[k] for e in ex]
            st.stack.append(vals[0] if all(v == vals[0] for v in vals) else ("in", "s%d" % k, b.id))
    return st


def eval_function(ctx, name, entry=None):
    entry = ctx.procs[name] if entry is None else entry
    f = build_cfg(ctx.insns, ctx.labels, ctx.tables, ctx.procs, name, entry)
    locs = ctx.locs.get(name, {})
    ev = Evaluator(f, ctx.insns, ctx.labels, locs, ctx.tables, entry)
    f.locs = locs
    f.ev = ev
    f.ctx = ctx
    exit_state = {}
    for bid in f.order:
        b = f.blocks[bid]
        b.retonly = bid != f.entry and retonly(ctx, b)
        if b.retonly:
            b.term = ("retonly",)
            continue
        if len(b.pred) == 1 and b.pred[0] in exit_state:
            st = exit_state[b.pred[0]]
        elif len(b.pred) > 1:
            st = join_state(f, b, exit_state)
        else:
            st = State()
        exit_state[bid] = ev.run(b, st)
        b.exit = exit_state[bid]
    f.exit_state = exit_state
    inline_returns(f)
    resolve_phis(f)
    return f


def inline_returns(f):
    """a jump to a block that only returns becomes a return carrying the value the jumping block leaves in ax"""
    for bid in list(f.blocks):
        b = f.blocks[bid]
        if b.retonly:
            continue
        t = b.term
        if t[0] == "goto" and f.blocks[t[1]].retonly:
            b.term = ("ret", b.exit.regs.get("ax"), b.exit.regs.get("dx"))
            b.succ = []
        elif t[0] == "if":
            new = []
            for k in (2, 3):
                if f.blocks[t[k]].retonly:
                    nid = max(f.blocks) + 1
                    nb = Block(nid, f.blocks[t[k]].start)
                    nb.insns = f.blocks[t[k]].insns
                    nb.term = ("ret", b.exit.regs.get("ax"), b.exit.regs.get("dx"))
                    nb.retonly = False
                    nb.exit = b.exit
                    nb.pred = [bid]
                    f.blocks[nid] = nb
                    f.order.append(nid)
                    new.append(nid)
                else:
                    new.append(t[k])
            b.term = ("if", t[1], new[0], new[1])
            b.succ = [new[1], new[0]]


def exprs_of(b):
    """all expressions of a block (statements and terminator)"""
    out = []
    for s in b.stmts:
        if s[0] == "set":
            out.append(s[1])
            out.append(s[2])
        elif s[0] == "call":
            out.extend(s[3])
            if isinstance(s[2], tuple):
                out.append(s[2])
    t = b.term
    if t[0] == "if":
        out.append(t[1])
    elif t[0] == "jtable":
        out.append(t[2])
    elif t[0] == "ret":
        if isinstance(t[1], tuple):
            out.append(t[1])
    elif t[0] == "tail":
        out.extend(x for x in t[2:4] if isinstance(x, tuple))
        out.extend(t[4])
    return out


def resolve_phis(f):
    """expressions mention ('in', reg, block) for values that depend on the path taken: give them a variable assigned at the end of every predecessor"""
    done = set()
    work = True
    while work:
        work = False
        for b in list(f.blocks.values()):
            for e in exprs_of(b):
                for x in walk(e):
                    if x[0] == "in" and (x[1], x[2]) not in done:
                        reg, J = x[1], x[2]
                        done.add((reg, J))
                        work = True
                        jb = f.blocks[J]
                        if not jb.pred:
                            raise Unsupported("register %s read before it is set" % reg)
                        for p in jb.pred:
                            pb = f.blocks[p]
                            if pb.retonly:
                                continue
                            if reg.startswith("s"):
                                v = pb.exit.stack[int(reg[1:])]
                            else:
                                v = pb.exit.regs.get(reg)
                                if v is None:
                                    v = ("in", reg, p)
                            pb.stmts.append(("set", ("phi", reg, J), v))


def count_uses(f):
    cnt = {}
    for b in f.blocks.values():
        for e in exprs_of(b):
            for x in walk(e):
                if x[0] == "res":
                    cnt[(x[1], x[2])] = cnt.get((x[1], x[2]), 0) + 1
    return cnt
