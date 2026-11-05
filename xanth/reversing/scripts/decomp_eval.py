"""Control-flow graph and symbolic evaluation of basic blocks for decomp.py."""
import re

from decomp import (L, STRIP, REG16, REG8, SEGS, JCC, K, is_k, simp, cmp_expr, negate_cond, parse_operand, Unsupported, walk)


class Block:
    def __init__(self, bid, start):
        self.id = bid
        self.start = start            # first instruction index
        self.insns = []
        self.succ = []                # block ids: [fallthrough/false, taken/true] for a conditional block
        self.pred = []
        self.stmts = []               # filled by evaluation
        self.term = None              # ('goto', b) ('if', cond, t, f) ('ret', expr) ('switch', idx, [(value, b)], default) ('end',)
        self.entry = None
        self.exit = None


class Func:
    def __init__(self, name):
        self.name = name
        self.blocks = {}
        self.order = []               # block ids in reverse post order
        self.entry = None
        self.args = {}                # arg offset -> name
        self.locals = {}
        self.calls = []               # callee names (statistics)
        self.ntemp = 0


CALLSIG = {}      # callee name -> number of argument words (learned from `add sp, N` after calls)


def clean_target(op):
    return STRIP.sub("", op.strip())


def learn_signatures(insns):
    for i, x in enumerate(insns[:-1]):
        if x.mn == "call" and x.ops and not x.ops[0].strip().startswith(("dword", "word", "es:", "cs:", "[", "bx", "ax")):
            nx = insns[i + 1]
            if nx.mn == "add" and nx.ops and nx.ops[0].strip() == "sp":
                try:
                    n = L.num(nx.ops[1])
                except Exception:
                    continue
                CALLSIG.setdefault(clean_target(x.ops[0]), set()).add(n // 2)


TRIVIAL = ("pop", "retf", "retn", "ret", "nop")


def chain_of(f, insns, start):
    """blocks that follow `start` unconditionally down to a return, with the number of real instructions; None if there is a branch or a loop"""
    chain, n, cur, seen = [], 0, start, set()
    while True:
        b = f.blocks[cur]
        if cur in seen or b.term is not None and b.term[0] in ("jtable", "tail"):
            return None
        seen.add(cur)
        chain.append(cur)
        last = insns[b.insns[-1]]
        n += sum(1 for i in b.insns if insns[i].mn not in TRIVIAL and not (insns[i].mn == "mov" and insns[i].ops[0].strip() in ("sp", "bp")))
        if last.mn in ("retf", "retn", "ret"):
            return chain, n
        if len(b.succ) != 1 or b.succ[0] is None or last.mn in JCC:
            return None
        cur = b.succ[0]


def duplicate_blocks(f, insns, max_tail=12):
    """a test that several paths share (each with its own flags) and a short block that only finishes the function are copied per predecessor"""
    changed = True
    rounds = 0
    while changed and rounds < 400:
        changed = False
        rounds += 1
        for bid in list(f.blocks):
            b = f.blocks.get(bid)
            if b is None or bid == f.entry or len(b.pred) < 2 or bid in f.jtable_targets:
                continue
            last = insns[b.insns[-1]]
            is_test = len(b.insns) == 1 and last.mn in JCC and last.mn not in ("jo", "jno")
            ch = None if is_test else chain_of(f, insns, bid)
            if not is_test and (ch is None or ch[1] * (len(b.pred) - 1) > 400 or ch[1] > max_tail):
                continue
            chain = [bid] if is_test else ch[0]
            preds = list(b.pred)
            for p in preds[1:]:
                pb = f.blocks[p]
                if bid not in pb.succ:
                    continue
                new_ids = {}
                for c in chain:
                    ob = f.blocks[c]
                    nb = Block(max(f.blocks) + 1, ob.start)
                    nb.insns = ob.insns
                    nb.succ = list(ob.succ)
                    nb.term = ob.term
                    nb.pred = []
                    f.blocks[nb.id] = nb
                    new_ids[c] = nb.id
                for c in chain:
                    nb = f.blocks[new_ids[c]]
                    nb.succ = [new_ids.get(x, x) if x is not None else None for x in nb.succ]
                    for sid in f.blocks[c].succ:
                        if sid is None:
                            continue
                        tgt = new_ids.get(sid, sid)
                        if sid in new_ids:
                            f.blocks[tgt].pred.append(new_ids[c])
                        else:
                            f.blocks[sid].pred.append(nb.id)
                pb.succ = [new_ids[bid] if x == bid else x for x in pb.succ]
                f.blocks[new_ids[bid]].pred.append(p)
                # the original loses this predecessor
                b.pred.remove(p)
                for c in chain:
                    for sid in f.blocks[c].succ:
                        if sid is not None and sid not in new_ids and sid in f.blocks:
                            op = f.blocks[sid].pred
                            # (the original chain keeps its own edge to sid; only the copy added one)
                changed = True
            if changed:
                break


def build_cfg(insns, labels, tables, procs, name, entry):
    idxs = L.reachable(insns, labels, tables, entry)
    s = set(idxs)
    leaders = {entry}
    for i in idxs:
        x = insns[i]
        if x.mn.startswith("j") or x.mn in ("loop", "loope", "loopne"):
            op = clean_target(x.ops[0]) if x.ops else ""
            m = re.match(r"^cs:(\w+)\[bx\]$", op)
            if m and m.group(1) in tables:
                for lab in tables[m.group(1)]:
                    if lab in labels:
                        leaders.add(labels[lab])
            elif op in labels:
                leaders.add(labels[op])
            if i + 1 in s:
                leaders.add(i + 1)
        elif x.mn in ("retf", "retn", "ret", "iret"):
            if i + 1 in s:
                leaders.add(i + 1)
    f = Func(name)
    cur = None
    blocks = []
    for i in idxs:
        if i in leaders or cur is None or insns[i - 1].mn.startswith("j") and i - 1 not in s:
            cur = Block(len(blocks), i)
            blocks.append(cur)
        cur.insns.append(i)
    by_start = {b.start: b for b in blocks}
    for b in blocks:
        last = insns[b.insns[-1]]
        mn = last.mn
        fall = None
        if b.insns[-1] + 1 in by_start:
            fall = by_start[b.insns[-1] + 1].id
        if mn == "jmp" and last.ops:
            op = clean_target(last.ops[0])
            m = re.match(r"^cs:(\w+)\[bx\]$", op)
            if m and m.group(1) in tables:
                b.succ = [by_start[labels[l]].id for l in tables[m.group(1)] if l in labels]
                b.term = ("jtable", m.group(1))
            elif re.match(r"^\$\+[23]$", op):
                b.succ = [fall] if fall is not None else []
            elif op in labels and labels[op] in by_start:
                b.succ = [by_start[labels[op]].id]
            else:
                b.term = ("tail", op)
        elif mn in JCC or mn in ("loop", "loope", "loopne", "jcxz"):
            op = clean_target(last.ops[0])
            tgt = by_start[labels[op]].id if op in labels and labels[op] in by_start else None
            b.succ = [fall, tgt]
        elif mn in ("retf", "retn", "ret", "iret"):
            b.succ = []
            b.term = ("ret",)
        else:
            b.succ = [fall] if fall is not None else []
    for b in blocks:
        for s_ in b.succ:
            if s_ is not None and b.id not in f.blocks.get(s_, Block(0, 0)).pred if False else True:
                pass
    f.blocks = {b.id: b for b in blocks}
    for b in blocks:
        for s_ in b.succ:
            if s_ is not None:
                f.blocks[s_].pred.append(b.id)
    f.entry = blocks[0].id
    f.jtable_targets = set()
    for b in blocks:
        if b.term and b.term[0] == "jtable":
            f.jtable_targets.update(b.succ)
    duplicate_blocks(f, insns)
    blocks = list(f.blocks.values())
    # reverse post order
    seen, post = set(), []

    def dfs(n):
        stack = [(n, iter([x for x in f.blocks[n].succ if x is not None]))]
        seen.add(n)
        while stack:
            node, it = stack[-1]
            nxt = next(it, None)
            if nxt is None:
                post.append(node)
                stack.pop()
            elif nxt not in seen:
                seen.add(nxt)
                stack.append((nxt, iter([x for x in f.blocks[nxt].succ if x is not None])))
    dfs(f.entry)
    f.order = list(reversed(post))
    return f
