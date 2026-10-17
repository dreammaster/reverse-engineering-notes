"""
Readable pseudo-C for the game-logic functions, from the decoded instructions (dump_insns.py JSON).

Per basic block a tiny symbolic evaluator keeps what each register holds (constants, loads, simple arithmetic) and the pending pushed
arguments, so calls print with their arguments and conditions print as expressions:

    objh_081(): ...
      L_2E053:
        if ((bitset_test(_game_flags, 7)) != 0) goto L_2E0C2;
        ...
        print_obj(2, (int16)(dword_CD92C >> 16))

It is a reading aid for hand-porting (control flow stays as labels/gotos), not compilable code. The output restates the game's logic, so keep it
local (game_out/), not in git.

    python pseudo_c.py insns.json OUT.c [name-filter-regex]
"""
import json
import re
import sys

REG32 = ["eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"]
ARGREG = ["eax", "edx", "ebx", "ecx"]
JCC = {"jz": "==", "jnz": "!=", "jb": "<u", "jnb": ">=u", "jbe": "<=u", "ja": ">u", "jl": "<", "jge": ">=", "jle": "<=", "jg": ">"}


def operands_text(text):
    t = text.split(";")[0].strip()
    parts = t.split(None, 1)
    if len(parts) < 2:
        return []
    ops, depth, cur = [], 0, ""
    for ch in parts[1]:
        if ch == "[":
            depth += 1
        elif ch == "]":
            depth -= 1
        if ch == "," and depth == 0:
            ops.append(cur.strip())
            cur = ""
        else:
            cur += ch
    ops.append(cur.strip())
    return ops


def clean(s):
    s = re.sub(r"\b(dword|word|byte) ptr\s+", "", s)
    s = re.sub(r"\bds:|\bcs:|\boffset\s+", "", s)
    return s


class F:
    def __init__(self, name, v, names):
        self.name, self.v, self.names = name, v, names
        self.out = []

    def expr_reg(self, regs, r, dt):
        base = regs.get(REG32[r & 7] if True else "", None)
        return base

    def run(self):
        insns = self.v["insns"]
        targets = set()
        for r in insns:
            if r["mn"] in JCC or r["mn"] == "jmp":
                if r["ops"] and r["ops"][0]["t"] == 7:
                    targets.add(r["ops"][0]["addr"])
                for t in r.get("jt", []):
                    targets.add(t)
        regs = {}
        stack = []          # pending pushed values (top last)
        last_cmp = None
        incoming = {}       # label -> register knowledge common to all forward jumps seen so far
        terminal = False    # previous instruction does not fall through

        def note_jump(tgt, regs_):
            if tgt in incoming:
                incoming[tgt] = {k: v for k, v in incoming[tgt].items() if regs_.get(k) == v}
            else:
                incoming[tgt] = dict(regs_)
        out = ["// %s @ 0x%X" % (self.name, self.v["ea"]), "%s() {" % self.name]
        for r in insns:
            ea, mn = r["ea"], r["mn"]
            if ea in targets:
                out.append("L_%X:" % ea)
                known = None if terminal else dict(regs)
                inc = incoming.get(ea)
                if known is None:
                    regs = dict(inc) if inc else {}
                elif inc is not None:
                    regs = {k: v for k, v in known.items() if inc.get(k) == v}
                else:
                    regs = {}       # reached by a backward jump we have not seen yet
                stack, last_cmp = [], None
            txt = clean(r["text"])
            ops = [clean(o) for o in operands_text(r["text"])]

            def val(i):
                o = ops[i]
                return regs.get(o, o)

            def setreg(i, v):
                o = ops[i]
                if o in REG32:
                    regs[o] = v
                else:
                    for k in list(regs):
                        if k in ("eax", "ecx", "edx", "ebx", "esi", "edi", "ebp") and k[1:] in o:
                            pass
                    regs[o] = v

            ind = "  "
            if mn == "mov" and len(ops) == 2:
                if ops[0] in REG32 or ops[0] in ("ax", "bx", "cx", "dx", "si", "di", "al", "bl", "cl", "dl", "ah", "bh", "ch", "dh"):
                    regs[ops[0]] = val(1)
                    if ops[0] in REG32:
                        for sub in (ops[0][1:], ops[0][1] + "l", ops[0][1] + "h"):
                            regs.pop(sub, None)
                else:
                    out.append(ind + "%s = %s;" % (ops[0], val(1)))
            elif mn in ("movsx", "movzx"):
                regs[ops[0]] = "%s(%s)" % ("(int)" if mn == "movsx" else "(uint)", val(1))
            elif mn == "xor" and ops[0] == ops[1]:
                regs[ops[0]] = "0"
            elif mn in ("add", "sub", "and", "or", "xor", "shl", "shr", "sar", "imul"):
                sym = {"add": "+", "sub": "-", "and": "&", "or": "|", "xor": "^", "shl": "<<", "shr": ">>u", "sar": ">>", "imul": "*"}[mn]
                if ops[0] == "esp":
                    n = int(ops[1].rstrip("h"), 16) if ops[1].endswith("h") else int(ops[1]) if ops[1].isdigit() else 0
                    if mn == "add":
                        for _ in range(n // 4):
                            if stack:
                                stack.pop()
                    continue
                if ops[0] in REG32:
                    regs[ops[0]] = "(%s %s %s)" % (val(0), sym, val(1))
                else:
                    out.append(ind + "%s %s= %s;" % (ops[0], sym, val(1)))
            elif mn in ("inc", "dec"):
                if ops[0] in REG32:
                    regs[ops[0]] = "(%s %s 1)" % (val(0), "+" if mn == "inc" else "-")
                else:
                    out.append(ind + "%s%s;" % (ops[0], "++" if mn == "inc" else "--"))
            elif mn == "lea":
                regs[ops[0]] = "&%s" % ops[1]
            elif mn == "push":
                stack.append(val(0))
            elif mn == "pop":
                if stack:
                    regs[ops[0]] = stack.pop()
                else:
                    regs.pop(ops[0], None)
            elif mn in ("cmp", "test"):
                last_cmp = (mn, val(0), val(1))
            elif mn in JCC:
                tgt = r["ops"][0]["addr"] if r["ops"] else 0
                if last_cmp:
                    kind, a, b = last_cmp
                    if kind == "test" and a == b:
                        cond = "%s %s 0" % (a, JCC[mn])
                    elif kind == "test":
                        cond = "(%s & %s) %s 0" % (a, b, JCC[mn])
                    else:
                        cond = "%s %s %s" % (a, JCC[mn], b)
                else:
                    cond = "flags %s" % JCC[mn]
                out.append(ind + "if (%s) goto L_%X;" % (cond, tgt))
                note_jump(tgt, regs)
            elif mn == "jmp":
                if "jt" in r:
                    out.append(ind + "switch (%s) { /* jump table */ %s }" % (val(0).split("[")[-1].rstrip("]") if False else ops[0], " ".join("case %d: goto L_%X;" % (k, t) for k, t in enumerate(r["jt"]))))
                else:
                    out.append(ind + "goto L_%X;" % r["ops"][0]["addr"])
                    note_jump(r["ops"][0]["addr"], regs)
            elif mn == "call":
                callee = ops[0] if ops else "?"
                args = [regs.get(x, x) for x in ARGREG]
                targ = ", ".join(reversed(stack)) if stack else ""
                shown = [a for a in args if a not in ARGREG]
                out.append(ind + "%s = %s(%s%s%s);" % ("eax", callee, ", ".join("%s=%s" % (n, regs[n]) for n in ARGREG if n in regs and regs[n] != n and n != "esp"), ("; stack: " + targ) if targ else "", ""))
                regs.pop("eax", None)
                for k in ("ecx", "edx", "ebx"):
                    regs.pop(k, None)
                regs["eax"] = "ret_%s" % callee
            elif mn == "retn":
                out.append(ind + "return %s;" % regs.get("eax", "eax"))
                regs, stack, last_cmp = {}, [], None
            elif mn in ("cwde",):
                regs["eax"] = "(int16)%s" % regs.get("ax", regs.get("eax", "ax"))
            else:
                out.append(ind + "/* %s */" % txt)
            terminal = mn in ("retn", "jmp")
        out.append("}")
        return "\n".join(out)


def main():
    funcs = json.load(open(sys.argv[1]))
    pat = re.compile(sys.argv[3]) if len(sys.argv) > 3 else None
    chunks = []
    for n, v in sorted(funcs.items(), key=lambda kv: kv[1]["ea"]):
        if pat and not pat.search(n):
            continue
        chunks.append(F(n, v, {}).run())
    open(sys.argv[2], "w", encoding="utf-8").write("\n\n".join(chunks) + "\n")
    print(len(chunks), "functions written to", sys.argv[2])


main()
