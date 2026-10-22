"""
Interprocedural liveness: which registers and flags each function reads from its caller (the inputs) and which of the
values it writes are read by some caller (the outputs).
"""
from model import (ARITH_FLAGS, ALLFLAGS, BYTES, CONDFLAGS, GENERAL, effects, is_weak, weak)

UNKNOWN_IN = frozenset(BYTES + ["si", "di", "bp", "es"])


def build_dataref_set(image, entries, ds_base=0xCE20):
    """function entries that appear as 16 bit words in the data segment or as immediates: they may be called indirectly"""
    esc = set()
    ents = set(entries)
    for i in range(ds_base, min(len(image) - 1, ds_base + 0x10000)):
        w = image[i] | (image[i + 1] << 8)
        if w in ents:
            esc.add(w)
    return esc


def load_hints(path):
    hints = {}
    for line in open(path):
        line = line.split("#")[0].split()
        if len(line) >= 2:
            hints[int(line[0], 16)] = [int(x, 16) for x in line[1:]]
    return hints


class Analysis:
    def __init__(self, prog, escaping, hints=None):
        self.prog = prog
        self.hints = hints or {}
        self.escaping = set(escaping)
        self.indirect_calls = {}     # function entry -> list of addresses with calli/jmpi
        self.solve()

    # -- per instruction ---------------------------------------------------------------------------------------------
    def site_effects(self, f, ins):
        """(uses, defs) of one instruction including callee summaries"""
        e = effects(ins)
        uses, defs = set(e.uses), set(e.defs)
        defs |= {weak(r) for r in defs if r in BYTES}
        op = ins.op
        if op == "call":
            g = self.prog.funcs.get(ins.target)
            if g:
                uses |= g.uses_in
                defs |= g.defs_all
        elif op in ("calli", "jmpi") and ins.addr in self.hints:
            for t in self.hints[ins.addr]:
                g = self.prog.funcs.get(t)
                if g:
                    uses |= g.uses_in
                    defs |= g.defs_all
        elif op in ("calli", "callf", "callfi"):
            uses |= UNKNOWN_IN
            defs |= GENERAL | ALLFLAGS | {"es"}
            if op == "calli" and ins.a is not None and ins.a.kind == "mem":
                uses |= set(r for r in e.uses)
        elif op in CONDFLAGS:
            uses |= CONDFLAGS[op]
        elif op in ("loop", "jcxz"):
            uses |= {"cl", "ch"}
            if op == "loop":
                defs |= {"cl", "ch"}
        elif op in ("loope", "loopne"):
            uses |= {"cl", "ch", "zf"}
            defs |= {"cl", "ch"}
        elif op == "int":
            uses |= {"al", "ah", "bl", "bh", "cl", "ch", "dl", "dh", "si", "di"}
            defs |= {"al", "ah", "bl", "bh", "cl", "ch", "dl", "dh", "cf", "zf", "es"}
        return uses, defs

    def terminator_uses(self, f, blk):
        """(uses, defs) contributed by the block terminator beyond the instruction itself"""
        if blk.term in ("tail", "jcc-tail"):
            g = self.prog.funcs.get(blk.tail)
            if g:
                return set(g.uses_in), set(g.defs_all)
        if blk.term == "indirect":
            last = blk.insns[-1]
            if last.addr in self.hints:
                u, d = set(), set()
                for t in self.hints[last.addr]:
                    g = self.prog.funcs.get(t)
                    if g:
                        u |= g.uses_in
                        d |= g.defs_all
                return u, d
            return set(UNKNOWN_IN), set(GENERAL | ALLFLAGS)
        return set(), set()

    # -- fixpoint ----------------------------------------------------------------------------------------------------
    def solve(self):
        funcs = self.prog.funcs
        for f in funcs.values():
            f.uses_in = set()
            f.defs_all = set()
            f.out = set(ALLFLAGS | GENERAL | {"es"}) if f.entry in self.escaping else set()
        changed = True
        rounds = 0
        while changed:
            changed = False
            rounds += 1
            for f in funcs.values():
                # defs
                d = set()
                for blk in f.blocks.values():
                    for ins in blk.insns:
                        _, df = self.site_effects(f, ins)
                        d |= df
                    _, td = self.terminator_uses(f, blk)
                    d |= td
                if not d <= f.defs_all:
                    f.defs_all |= d
                    changed = True
                # liveness
                livein = self.liveness(f)
                entry_live = livein.get(f.entry, set())
                if not entry_live <= f.uses_in:
                    f.uses_in |= entry_live
                    changed = True
            # propagate caller liveness into callee outputs
            for f in funcs.values():
                live_after = self.live_after_calls(f)
                for site, live in live_after.items():
                    callee, kind = site
                    g = funcs.get(callee)
                    if not g:
                        continue
                    need = live & g.defs_all
                    if not need <= g.out:
                        g.out |= need
                        changed = True
        self.rounds = rounds

    def block_live_out(self, f, blk, livein):
        """live registers after the last instruction of the block (terminator effects included)"""
        live = set()
        for t in blk.succ:
            live |= livein.get(t, set())
        if blk.term == "ret":
            live |= f.out
        elif blk.term in ("tail", "jcc-tail", "indirect"):
            tu, td = self.terminator_uses(f, blk)
            live |= (set(f.out) - td) | tu
        return live

    def liveness(self, f):
        livein = {s: set() for s in f.blocks}
        changed = True
        while changed:
            changed = False
            for s in reversed(f.order):
                blk = f.blocks[s]
                live = self.block_live_out(f, blk, livein)
                live = self.walk_block(f, blk, live)
                if live != livein[s]:
                    livein[s] = live
                    changed = True
        return livein

    def walk_block(self, f, blk, live, record=None):
        """live registers before the block given the live set after it; `record` collects live sets after call sites"""
        live = set(live)
        if record is not None and blk.tail is not None:
            record[(blk.tail, "tail")] = set(f.out)
        for ins in reversed(blk.insns):
            uses, defs = self.site_effects(f, ins)
            if ins.op == "call" and record is not None and ins.target in self.prog.funcs:
                record[(ins.target, ins.addr)] = set(live)
            live = (live - defs) | uses
        return live

    def live_after_calls(self, f):
        record = {}
        livein = self.liveness(f)
        for s in f.order:
            blk = f.blocks[s]
            live = self.block_live_out(f, blk, livein)
            self.walk_block(f, blk, live, record)
        return record


def sig_in(f):
    return {r for r in f.uses_in if not is_weak(r) and r not in ("df", "ds")}


def sig_out(f):
    return {r for r in f.out if not is_weak(r) and r not in ("df", "ds")}


def sig_junk(f):
    return {r for r in f.defs_all if not is_weak(r) and r not in ("df", "ds")} - sig_out(f)
