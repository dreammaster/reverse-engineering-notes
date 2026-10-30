"""
Builds a readable "digest" of every Xanth object handler (objh_NNN) and verb default (cmd*) from the exported xanth.asm, so the 448 handlers can be read
(and eventually transpiled) without wading through 16-bit assembly.

A handler is `objh_NNN(phase)`; it switches on the phase argument and on `selectedCommand` (the verb) with chains of cmp/sub/dec + jz.  This script walks every path
through the function with a tiny symbolic state:

  * ax/bx/cx/dx hold constants, global names, the symbolic phase (`phase`), the verb (`cmd`) or `ret(fn)`;
  * `sub ax,N` / `dec ax` / `cmp ax,N` followed by jz/jnz/ja... become conditions such as `cmd==0x13` or `phase==6`;
  * calls are printed with the constants/globals pushed for them (C argument order); a pushed (dx:ax) pair with dx == 0xF0BB is a SHANSTR message id and is shown as
    `msg(BB:index)`; a pushed (offset, ds) pair is shown as `&name`;
  * stores to globals are printed as `name = value`;
  * paths with identical effects are merged ("when A | B: effects"); within a path, only == tests are kept per tracked variable (else `cmd:other`).

    python extract_xanth_digest.py ../../xanth.asm ../analysis/xanth_handler_digest.txt [objh_100 ...]
"""
import re
import sys

MAXPATHS = 150
CMD = {}
truncated = set()


def load_enums():
    import os
    p = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "analysis", "xanth_enums.tsv")
    if os.path.exists(p):
        for l in open(p, encoding="utf-8").read().splitlines()[1:]:
            f = l.split(chr(9))
            if len(f) == 3 and f[0] == "CMD":
                CMD[int(f[1])] = f[2][4:]


JCC = {"jz": "==", "je": "==", "jnz": "!=", "jne": "!=", "ja": ">", "jnbe": ">", "jg": ">", "jnle": ">", "jae": ">=", "jnb": ">=", "jge": ">=", "jnl": ">=",
       "jb": "<", "jnae": "<", "jl": "<", "jnge": "<", "jbe": "<=", "jna": "<=", "jle": "<=", "jng": "<=", "jc": "<", "jnc": ">=", "js": "<0", "jns": ">=0"}
NEG = {"==": "!=", "!=": "==", ">": "<=", "<=": ">", ">=": "<", "<": ">=", "<0": ">=0", ">=0": "<0"}
REG16 = {"ax", "bx", "cx", "dx", "si", "di", "bp", "sp"}
REG8 = {"al": "ax", "ah": "ax", "bl": "bx", "bh": "bx", "cl": "cx", "ch": "cx", "dl": "dx", "dh": "dx"}


def parse(path):
    funcs, cur = {}, None
    for line in open(path, encoding="utf-8", errors="replace"):
        m = re.match(r"^(\S+)\s+proc (near|far)", line)
        if m:
            cur = m.group(1)
            funcs[cur] = []
            continue
        if cur and re.match(r"^%s\s+endp" % re.escape(cur), line):
            cur = None
            continue
        if cur:
            funcs[cur].append(line.rstrip("\n"))
    return funcs


def tokens(body):
    """-> (insns, labels): insns = [(mnem, [ops], comment)], labels = name -> insn index"""
    insns, labels = [], {}
    for line in body:
        m = re.match(r"^(\w+):", line)
        if m:
            labels[m.group(1)] = len(insns)
            continue
        if not line.startswith((" ", "\t")):
            continue
        code, _, cmt = line.partition(";")
        code = code.strip()
        if not code or code.startswith(("assume", "align", "db ", "dw ", "dd ", "public")) or re.match(r"^\w+\s*=\s", code):
            continue
        parts = code.split(None, 1)
        mn = parts[0]
        ops = [o.strip() for o in parts[1].split(",")] if len(parts) > 1 else []
        if mn == "short" or mn == "far":
            continue
        insns.append((mn, ops, cmt.strip()))
    return insns, labels


def clean(op):
    op = re.sub(r"\b(short|near|far ptr|word ptr|byte ptr|dword ptr|offset)\s+", lambda m: "&" if m.group(1) == "offset" else "", op)
    return op.strip()


def immval(op):
    op = op.strip()
    m = re.match(r"^(-?[0-9A-F]+)h$", op)
    if m:
        v = int(m.group(1), 16)
        return v
    if re.match(r"^-?\d+$", op):
        return int(op)
    m = re.match(r"^'(.)'$", op)
    return ord(m.group(1)) if m else None


def hexs(v):
    return str(v) if 0 <= v < 10 else "0x%X" % v


class State:
    def __init__(self):
        self.regs = {}
        self.cmp = None
        self.pend = []
        self.conds = []
        self.seq = []
        self.other = set()
        self.ev = []
        self.seen = set()

    def copy(self):
        s = State()
        s.regs = dict(self.regs)
        s.cmp = self.cmp
        s.pend = list(self.pend)
        s.conds = list(self.conds)
        s.seq = list(self.seq)
        s.other = set(self.other)
        s.ev = list(self.ev)
        s.seen = set(self.seen)
        return s


def val(st, op):
    """symbolic value of an operand: int, ('sym', name, delta), or string"""
    op = clean(op)
    v = immval(op)
    if v is not None:
        return v
    r = REG8.get(op, op)
    if r in REG16 or r in REG8.values():
        return st.regs.get(r, r)
    if op.startswith("&"):
        return op
    m = re.match(r"^\[bp\+(arg_\w+)\]$", op)
    if m:
        return ("sym", "phase", 0) if m.group(1) == "arg_0" else m.group(1)
    if op == "selectedCommand":
        return ("sym", "cmd", 0)
    return op


def show(v):
    if isinstance(v, int):
        return hexs(v)
    if isinstance(v, tuple):
        return v[1] if v[2] == 0 else "%s-%s" % (v[1], hexs(v[2]))
    return str(v)


def addevent(st, text):
    st.ev.append(text)
    st.seq.append(("e", text))


def addcond(st, k, op, v):
    """record a branch condition; for the tracked phase/cmd variables only == is kept, anything else is 'cmd:other' (once per path)"""
    st.conds.append((k, op, v))
    if k in ("cmd", "phase"):
        if op == "==":
            t = "%s==%s" % (k, cname(k, v))
            if ("c", k + ":other") in st.seq:
                st.seq.remove(("c", k + ":other"))
                st.other.discard(k)
        elif k in st.other or any(c[0] == k and c[1] == "==" for c in st.conds[:-1]):
            return
        else:
            st.other.add(k)
            t = "%s:other" % k
    else:
        t = "%s%s%s" % (k, op, cname(k, v) if v != "" else "")
    if k in ("cmd", "phase") and ("c", t) in st.seq:
        return
    if st.seq and st.seq[-1] == ("e", k):
        st.seq.pop()                      # the tested call is the condition itself: don't print it twice
    st.seq.append(("c", t))


def merge_args(vals):
    """vals in memory order (last pushed first) -> C argument list text"""
    out, i = [], 0
    while i < len(vals):
        v = vals[i]
        nxt = vals[i + 1] if i + 1 < len(vals) else None
        if isinstance(nxt, int) and 0xF000 <= nxt <= 0xF0FF and (isinstance(v, int) or v == "?"):
            out.append("msg(%d:%s)" % (nxt & 0xFF, hexs(v) if isinstance(v, int) else "?"))
            i += 2
        elif isinstance(nxt, int) and 0xF000 <= nxt <= 0xF0FF and not isinstance(v, int):
            out.append("msg(%d:%s)" % (nxt & 0xFF, show(v)))
            i += 2
        elif nxt == "ds" and isinstance(v, str) and v.startswith("&"):
            out.append(v)
            i += 2
        else:
            out.append(show(v))
            i += 1
    return out


def cname(k, v):
    if k == "cmd" and isinstance(v, int) and v in CMD:
        return CMD[v]
    return hexs(v) if isinstance(v, int) else str(v)


def cond_text(conds):
    byk = {}
    for k, op, v in conds:
        byk.setdefault(k, []).append((op, v))
    parts = []
    for k, lst in byk.items():
        eq = [(op, v) for op, v in lst if op == "=="]
        if eq:
            parts.append("%s==%s" % (k, cname(k, eq[0][1])))
        elif k in ("cmd", "phase"):
            parts.append("%s:other" % k)
        else:
            parts.extend("%s%s%s" % (k, op, hexs(v) if isinstance(v, int) else v) for op, v in dict.fromkeys(lst))
    return " & ".join(parts) if parts else "always"


STUBS = {}


def find_stubs(funcs):
    """RTLink overlay stubs `call rtlink_thunk / jmp target`: calls to them are shown as calls to the real target"""
    for n, body in funcs.items():
        code = [l.split(";")[0].strip() for l in body if l.startswith((" ", chr(9))) and l.split(";")[0].strip()]
        code = [c for c in code if not c.startswith(("assume", "align", "db "))]
        if len(code) == 2 and code[0].split()[-1] == "rtlink_thunk" and code[1].startswith("jmp"):
            STUBS[n] = clean(code[1].split(None, 1)[1])


def run(name, body, calls_known):
    insns, labels = tokens(body)
    results = []
    stack = [(0, State())]
    npaths = 0
    while stack:
        i, st = stack.pop()
        while True:
            if i >= len(insns):
                results.append((st, "falls off"))
                break
            mn, ops, cmt = insns[i]
            i += 1
            if mn in ("retf", "retn", "ret"):
                results.append((st, show(st.regs.get("ax", "?"))))
                break
            if mn == "jmp":
                tgt = clean(ops[0]) if ops else ""
                if tgt in labels:
                    if tgt in st.seen:
                        addevent(st, "loop")
                        results.append((st, "loop"))
                        break
                    st.seen.add(tgt)
                    i = labels[tgt]
                    continue
                addevent(st, "tail %s" % tgt.replace("far ptr ", ""))
                results.append((st, "tail"))
                break
            if mn in JCC:
                tgt = clean(ops[0])
                op = JCC[mn]
                c = st.cmp
                taken, fall = st.copy(), st
                if c:
                    for s, o in ((taken, op), (fall, NEG[op])):
                        lhs, rhs = c
                        if isinstance(lhs, tuple) and isinstance(rhs, int):
                            o2 = o
                            addcond(s, lhs[1], o2, rhs)
                        elif rhs is None:
                            addcond(s, show(lhs), o, 0 if o not in ("<0", ">=0") else "")
                        else:
                            addcond(s, show(lhs), o, rhs if isinstance(rhs, int) else show(rhs))
                if tgt in labels and tgt not in taken.seen:
                    if npaths < MAXPATHS:
                        taken.seen.add(tgt)
                        stack.append((labels[tgt], taken))
                        npaths += 1
                    else:
                        truncated.add(name)
                continue
            # --- data movement ---
            d = REG8.get(clean(ops[0]), clean(ops[0])) if ops else ""
            if mn == "push":
                o = clean(ops[0])
                if o in ("bp", "si", "di", "es", "cs") and not st.pend and i < 8:
                    continue
                st.pend.append(val(st, o))
            elif mn == "pop":
                o = clean(ops[0])
                if st.pend:
                    st.pend.pop()
                st.regs[o] = "?"
            elif mn == "mov":
                src = val(st, ops[1])
                dst = clean(ops[0])
                if dst in REG16 or dst in REG8:
                    if dst in REG8 and isinstance(st.regs.get(REG8[dst]), tuple):
                        st.regs[REG8[dst]] = "?"
                    st.regs[d] = src
                elif dst not in ("sp", "es") and not dst.startswith("[bp") and not dst.startswith("es:") or dst.startswith("es:"):
                    if dst not in ("es",):
                        addevent(st, "%s = %s" % (dst, show(src)))
            elif mn == "xor" and clean(ops[0]) == clean(ops[1]):
                st.regs[d] = 0
            elif mn in ("sub", "add", "dec", "inc", "cmp", "test", "or", "and"):
                a = val(st, ops[0]) if ops else None
                b = val(st, ops[1]) if len(ops) > 1 else 1
                dst = clean(ops[0])
                isreg = dst in REG16 or dst in REG8
                if mn == "cmp":
                    if isinstance(a, tuple) and isinstance(b, int):
                        st.cmp = (a, b + a[2])
                    else:
                        st.cmp = (a, b)
                elif mn in ("test", "or") and len(ops) > 1 and clean(ops[0]) == clean(ops[1]):
                    st.cmp = (a, None)
                elif mn == "test":
                    st.cmp = (("%s&%s" % (show(a), show(b))), 0)
                elif isreg and mn in ("sub", "dec") and isinstance(a, tuple) and isinstance(b, int):
                    na = ("sym", a[1], a[2] + b)
                    st.regs[d] = na
                    st.cmp = (na, na[2])
                    st.cmp = (na, na[2])
                elif isreg and mn in ("add", "inc") and isinstance(a, tuple) and isinstance(b, int):
                    st.regs[d] = ("sym", a[1], a[2] - b)
                    st.cmp = None
                elif isreg and mn in ("sub", "add", "and", "or") and isinstance(a, int) and isinstance(b, int):
                    st.regs[d] = {"sub": a - b, "add": a + b, "and": a & b, "or": a | b}[mn] & 0xFFFF
                elif isreg:
                    if mn in ("sub", "dec"):
                        st.cmp = (a, b)
                    else:
                        st.cmp = None
                    st.regs[d] = "(%s %s %s)" % (show(a), {"sub": "-", "dec": "-", "add": "+", "inc": "+", "and": "&", "or": "|"}[mn], show(b))
                else:
                    if mn != "cmp":
                        addevent(st, "%s %s %s" % (dst, mn, show(b)) if len(ops) > 1 else "%s %s" % (mn, dst))
                    st.cmp = None
            elif mn in ("call",):
                t = clean(ops[0]).replace("far ptr ", "")
                t = t[2:] if t.startswith("j_") else t
                for _ in range(3):
                    t = STUBS.get(t, t)
                args = merge_args(list(reversed(st.pend)))
                if t != "rtlink_thunk":
                    addevent(st, "%s(%s)" % (t, ", ".join(args)))
                st.pend = []
                st.regs["ax"] = "%s(%s)" % (t, ", ".join(args))
                st.regs["dx"] = "?"
                st.cmp = None
            elif mn == "add" or (mn == "mov" and False):
                pass
            elif mn == "lea" or mn == "les" or mn == "lds":
                st.regs[d] = "?"
            elif mn in ("cbw", "cwd", "neg", "not", "shl", "shr", "sar", "sal", "mul", "imul", "div", "idiv", "xchg"):
                st.regs["ax"] = "?"
                st.regs["dx"] = "?"
                st.cmp = None
            elif mn == "cmp":
                pass
    return results


def digest(name, body, calls_known):
    res = run(name, body, calls_known)
    root = {}
    for st, rv in res:
        node = root
        for item in st.seq + [("r", rv)]:
            node = node.setdefault(item, {})
    lines = []

    def emit(node, depth):
        for (kind, text), child in node.items():
            pad = "  " * depth
            if kind == "c":
                lines.append("%sif %s:" % (pad, text))
                emit(child, depth + 1)
            elif kind == "e":
                lines.append("%s%s" % (pad, text))
                emit(child, depth)
            else:
                lines.append("%s-> %s" % (pad, text))

    emit(root, 1)
    if name in truncated:
        lines.append("  [truncated: more than %d paths]" % MAXPATHS)
    return lines


OWNERS = {}


def load_owners():
    """handler index -> 'id name' of the objects using it (from analysis/xanth_objects.tsv, made by xanth_objects.py)"""
    import os
    p = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "analysis", "xanth_objects.tsv")
    if not os.path.exists(p):
        return
    for l in open(p, encoding="utf-8").read().splitlines()[1:]:
        f = l.split(chr(9))
        if len(f) > 7 and f[7] not in ("", "0"):
            OWNERS.setdefault(int(f[7]), []).append("%s %s" % (f[0], f[2]))


def main():
    load_enums()
    load_owners()
    funcs = parse(sys.argv[1])
    find_stubs(funcs)
    only = sys.argv[3:]
    out = open(sys.argv[2], "w", encoding="utf-8") if not only else sys.stdout
    names = [n for n in funcs if re.match(r"^(objh_\d+|exit_fn_\d+|cmd\w+|verbdef\w*)$", n)]
    names.sort()
    if only:
        names = only
    for n in names:
        m = re.match(r"^objh_(\d+)$", n)
        own = OWNERS.get(int(m.group(1)), []) if m else []
        out.write("%s%s\n" % (n, ("   # objects: " + ", ".join(own[:6]) + (" ..." if len(own) > 6 else "")) if own else ""))
        for l in digest(n, funcs[n], funcs):
            out.write(l + "\n")
        out.write("\n")
    print("handlers digested:", len(names), "truncated at %d paths:" % MAXPATHS, sorted(truncated), file=sys.stderr)


main()
