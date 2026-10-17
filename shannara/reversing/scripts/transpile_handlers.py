"""
Static recompiler: turns the decoded x86 of the game-logic functions (dump_insns.py JSON) into C++ that runs on port/src/machine.h.

    python transpile_handlers.py insns.json OUT_DIR [retn_map.txt]

Every instruction becomes one or a few C++ statements that update the Machine's registers, flags, stack and flat memory exactly as the
original does (the handlers use only ~45 distinct mnemonics); jumps become `goto`, jump tables become `switch`, calls become
`m.push(return); m.call(address)` (transpiled function or shim). The generated source is derived from the game's code and is NOT
committed (git-ignored); only this generator is. Output: gen_NN.cpp files, each with `registerGenN(Machine&)`, plus gen_registry.cpp.

Correctness is checked by differential tests against the unicorn oracle (retail_game.py), see port/tests/engine_diff.py.
"""
import json
import os
import re
import sys

REG32 = ["eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"]
REG16 = ["ax", "cx", "dx", "bx", "sp", "bp", "si", "di"]
REG8 = ["al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"]
TY = {0: "uint8_t", 1: "uint16_t", 2: "uint32_t"}
MASK = {0: 0xFF, 1: 0xFFFF, 2: 0xFFFFFFFF}
BITS = {0: 8, 1: 16, 2: 32}

# boundary functions: never emitted, calls go to shims even if they are inside the game-layer address range
SHIM_OVERRIDE = {"load_game?", "save_game?", "serialize_state?", "save_io_block?", "start_encounter?", "combat_loop?", "combat_hook_notify?",
                 "delay?", "delay_animate?", "wait_ticks?", "show_dialog?", "make_voice?", "make_sound", "refresh_room?", "send_event?",
                 "send_event_code?", "dispatch_event?", "play_music_entry?", "set_room_music?", "journal_render_page?", "setup_pic_regions?",
                 "disable_pic_regions?", "load_pic_sequence?", "play_cutscene?", "sub_73D1C", "sub_101A0", "sub_22D50", "sub_10FA0",
                 "wprintf", "wputchar", "sprintf_"}

JCC = {"jz": "m.zf", "jnz": "!m.zf", "jb": "m.cf", "jnb": "!m.cf", "jbe": "m.cond_be()", "ja": "!m.cond_be()",
       "jl": "m.cond_l()", "jge": "!m.cond_l()", "jle": "m.cond_le()", "jg": "!m.cond_le()", "js": "m.sf", "jns": "!m.sf",
       "jae": "!m.cf", "jc": "m.cf", "jnc": "!m.cf", "je": "m.zf", "jne": "!m.zf"}
SETCC = {"setz": "m.zf", "setnz": "!m.zf", "setl": "m.cond_l()", "setnl": "!m.cond_l()", "setle": "m.cond_le()", "setnle": "!m.cond_le()",
         "setb": "m.cf", "setnb": "!m.cf", "setbe": "m.cond_be()", "seta": "!m.cond_be()"}


class Gen:
    def __init__(self, funcs, shimnames, retn):
        self.funcs = funcs
        self.entries = {v["ea"]: n for n, v in funcs.items()}
        self.shimnames = shimnames
        self.retn = retn
        self.boundary = {}      # address -> name of called functions that are not transpiled
        self.pending = []       # synthetic tail functions discovered while generating

    # ---- operands ----------------------------------------------------------------------------------------------------------
    def ea(self, op):
        t = op["t"]
        parts = []
        if "sib" in op:
            base, index, scale = op["sib"]
            if not (t == 2):              # o_mem with SIB has no base register (mod 00, base 101)
                parts.append("m.%s" % REG32[base])
            if index != 4:
                parts.append("m.%s * %d" % (REG32[index], scale))
            if op["addr"]:
                parts.append("0x%Xu" % op["addr"])
        elif t == 2:
            return "0x%Xu" % op["addr"]
        else:
            parts.append("m.%s" % REG32[op["phrase"]])
            if t == 4 and op["addr"]:
                parts.append("0x%Xu" % op["addr"])
        return "(uint32_t)(%s)" % " + ".join(parts) if parts else "0u"

    def rd(self, op, dt=None):
        t = op["t"]
        dt = op["dt"] if dt is None else dt
        if t == 1:
            r = op["reg"]
            if dt == 0 and r >= 16:
                r -= 16                   # IDA numbers the byte registers 16..23 (al cl dl bl ah ch dh bh)
            if dt == 1 and r >= 8:
                raise ValueError("segment register operand")
            if dt == 2:
                return "m.%s" % REG32[r]
            if dt == 1:
                return "(uint16_t)m.%s" % REG32[r]
            return "(uint8_t)m.%s" % REG32[r & 3] if r < 4 else "(uint8_t)(m.%s >> 8)" % REG32[r & 3]
        if t == 5:
            return "(%s)0x%Xu" % (TY[dt], op["val"] & MASK[dt])
        if t in (2, 3, 4):
            return "m.r%d(%s)" % (BITS[dt], self.ea(op))
        raise ValueError("operand type %s" % t)

    def wr(self, op, val, dt=None):
        t = op["t"]
        dt = op["dt"] if dt is None else dt
        if t == 1:
            r = op["reg"]
            if dt == 0 and r >= 16:
                r -= 16
            if dt == 1 and r >= 8:
                raise ValueError("segment register operand")
            if dt == 2:
                return "m.%s = %s;" % (REG32[r], val)
            if dt == 1:
                return "m.%s = (m.%s & 0xFFFF0000u) | (uint16_t)(%s);" % (REG32[r], REG32[r], val)
            if r < 4:
                return "m.%s = (m.%s & 0xFFFFFF00u) | (uint8_t)(%s);" % (REG32[r], REG32[r], val)
            return "m.%s = (m.%s & 0xFFFF00FFu) | ((uint32_t)(uint8_t)(%s) << 8);" % (REG32[r & 3], REG32[r & 3], val)
        if t in (2, 3, 4):
            return "m.w%d(%s, %s);" % (BITS[dt], self.ea(op), val)
        raise ValueError("write operand type %s" % t)

    # ---- instructions ------------------------------------------------------------------------------------------------------
    def call_stmt(self, r, nxt):
        op = r["ops"][0]
        if op["t"] == 7:
            a = op["addr"]
            if a not in self.entries:
                self.boundary[a] = self.shimnames.get(a, "?")
            return "m.push(0x%Xu); m.call(0x%Xu);" % (nxt, a)
        return "{ uint32_t t_ = %s; m.push(0x%Xu); m.call(t_); }" % (self.rd(op, 2), nxt)

    def insn(self, r, labels, fn_eas, fname):
        mn, ops = r["mn"], r["ops"]
        nxt = r["ea"] + r["size"]
        text = r["text"]
        pre = text.split()[0] if text.split() and text.split()[0] in ("rep", "repe", "repne") else ""
        o = ops
        dt0 = o[0]["dt"] if o else 2

        def alu(fn):
            T = TY[dt0]
            return self.wr(o[0], "m.%s<%s>(%s, %s)" % (fn, T, self.rd(o[0]), self.rd(o[1], dt0)))

        if mn == "mov":
            return self.wr(o[0], self.rd(o[1], dt0))
        if mn == "push":
            return "m.push(%s);" % self.rd(o[0], 2)
        if mn == "pop":
            return self.wr(o[0], "m.pop()", 2)
        if mn == "call":
            return self.call_stmt(r, nxt)
        if mn == "retn":
            n = o[0]["val"] if o else 0
            return "m.esp += %d; return;" % (4 + n)
        if mn == "add": return alu("add")
        if mn == "sub": return alu("sub")
        if mn == "and": return alu("and_")
        if mn == "or": return alu("or_")
        if mn == "xor": return alu("xor_")
        if mn == "adc": return alu("adc")
        if mn == "sbb": return alu("sbb")
        if mn == "cmp":
            return "(void)m.sub<%s>(%s, %s);" % (TY[dt0], self.rd(o[0]), self.rd(o[1], dt0))
        if mn == "test":
            return "(void)m.and_<%s>(%s, %s);" % (TY[dt0], self.rd(o[0]), self.rd(o[1], dt0))
        if mn == "inc": return self.wr(o[0], "m.inc<%s>(%s)" % (TY[dt0], self.rd(o[0])))
        if mn == "dec": return self.wr(o[0], "m.dec<%s>(%s)" % (TY[dt0], self.rd(o[0])))
        if mn == "neg": return self.wr(o[0], "m.neg<%s>(%s)" % (TY[dt0], self.rd(o[0])))
        if mn == "not": return self.wr(o[0], "(%s)~%s" % (TY[dt0], self.rd(o[0])))
        if mn in ("shl", "shr", "sar"):
            cnt = self.rd(o[1], 0) if len(o) > 1 else "1"
            return self.wr(o[0], "m.%s<%s>(%s, %s)" % (mn, TY[dt0], self.rd(o[0]), cnt))
        if mn == "lea":
            return self.wr(o[0], "(%s)%s" % (TY[dt0], self.ea(o[1])))
        if mn == "movsx":
            sdt = o[1]["dt"]
            sty = {0: "int8_t", 1: "int16_t", 2: "int32_t"}[sdt]
            return self.wr(o[0], "(%s)(%s)(%s)" % (TY[dt0], sty, self.rd(o[1], sdt)))
        if mn == "movzx":
            return self.wr(o[0], "(%s)(%s)" % (TY[dt0], self.rd(o[1], o[1]["dt"])))
        if mn == "cwde":
            return "m.eax = (uint32_t)(int32_t)(int16_t)m.eax;"
        if mn == "cwd":
            return "m.edx = (m.edx & 0xFFFF0000u) | ((int16_t)m.eax < 0 ? 0xFFFFu : 0u);"
        if mn == "movsw": return "m.movsw(%s);" % ("true" if pre else "false")
        if mn == "cdq":
            return "m.edx = (int32_t)m.eax < 0 ? 0xFFFFFFFFu : 0u;"
        one_operand = "," not in text.split(";")[0]    # IDA lists the implicit eax first for one-operand forms (idiv esi -> [eax, esi])
        if mn == "imul":
            if one_operand:
                o = [o[-1]]
            if len(o) == 1:
                return "{ int64_t p_ = (int64_t)(int32_t)m.eax * (int32_t)%s; m.eax = (uint32_t)p_; m.edx = (uint32_t)(p_ >> 32); m.cf = m.of = p_ != (int32_t)p_; }" % self.rd(o[0], 2)
            a, b = (o[1], o[2]) if len(o) == 3 else (o[0], o[1])
            return "{ int64_t p_ = (int64_t)(int32_t)%s * (int32_t)%s; %s m.cf = m.of = p_ != (int32_t)p_; }" % (
                self.rd(a, 2), self.rd(b, 2), self.wr(o[0], "(uint32_t)p_", 2))
        if mn == "idiv":
            return "{ int64_t n_ = ((int64_t)(int32_t)m.edx << 32) | m.eax; int32_t d_ = (int32_t)%s; if (!d_) throw FormatError(\"divide by zero\"); m.eax = (uint32_t)(n_ / d_); m.edx = (uint32_t)(n_ %% d_); }" % self.rd(o[-1], 2)
        if mn == "div":
            return "{ uint64_t n_ = ((uint64_t)m.edx << 32) | m.eax; uint32_t d_ = %s; if (!d_) throw FormatError(\"divide by zero\"); m.eax = (uint32_t)(n_ / d_); m.edx = (uint32_t)(n_ %% d_); }" % self.rd(o[-1], 2)
        if mn in SETCC:
            return self.wr(o[0], "(%s) ? 1 : 0" % SETCC[mn], 0)
        if mn in JCC or mn == "jmp":
            return self.jump(r, labels, fn_eas)
        if mn == "movsd": return "m.movsd(%s);" % ("true" if pre else "false")
        if mn == "movsb": return "m.movsb(%s);" % ("true" if pre else "false")
        if mn == "scasb": return "m.scasb(%s);" % ("true" if pre else "false")
        if mn == "cmpsb": return "m.cmpsb(%s);" % ("true" if pre else "false")
        if mn in ("verr", "db", "nop"):
            return "/* %s: not code */" % text
        raise ValueError("unhandled instruction %s in %s: %s" % (mn, fname, text))

    def jump(self, r, labels, fn_eas):
        mn, ops = r["mn"], r["ops"]
        cond = JCC.get(mn)
        op = ops[0]
        if "jt" in r:
            idx = ops[0]["sib"][1]
            lines = ["switch (m.%s) {" % REG32[idx]]
            for k, tgt in enumerate(r["jt"]):
                lines.append("  case %d: %s break;" % (k, self.goto(tgt, labels, fn_eas)))
            lines.append("  default: throw FormatError(\"jump table index out of range\");")
            lines.append("}")
            return "\n".join(lines)
        if op["t"] != 7:
            raise ValueError("indirect jump without table: %s" % r["text"])
        g = self.goto(op["addr"], labels, fn_eas)
        return ("if (%s) { %s }" % (cond, g)) if cond else g

    def goto(self, tgt, labels, fn_eas):
        if tgt in fn_eas:
            labels.add(tgt)
            return "goto L_%X;" % tgt
        if tgt in self.entries:
            return "{ m.call(0x%Xu); return; }" % tgt     # tail jump to another function
        if tgt in self.shimnames:                   # tail jump to a boundary function (shim)
            self.boundary[tgt] = self.shimnames[tgt]
            return "{ m.call(0x%Xu); return; }" % tgt
        # jump into the middle of another function (shared return / tail code): give that code its own entry point
        for n, v in self.funcs.items():
            if any(r["ea"] == tgt for r in v["insns"]):
                tail = [r for r in v["insns"] if r["ea"] >= tgt]
                self.funcs["tail_%X" % tgt] = {"ea": tgt, "insns": tail}
                self.entries[tgt] = "tail_%X" % tgt
                self.pending.append("tail_%X" % tgt)
                return "{ m.call(0x%Xu); return; }" % tgt
        raise ValueError("jump to 0x%X outside the function and not a function entry" % tgt)

    def function(self, name, v):
        insns = v["insns"]
        fn_eas = {r["ea"] for r in insns}
        labels = set()
        body = []
        for r in insns:
            body.append((r["ea"], "  // %s\n  if (m.tracing) m.traceInsn(0x%Xu);\n  %s" % (r["text"].replace("\n", " ").replace("*/", "* /"), r["ea"], self.insn(r, labels, fn_eas, name))))
        out = ["// %s @ 0x%X" % (name, v["ea"]), "static void f_%X(Machine &m) {" % v["ea"]]
        for ea, text in body:
            if ea in labels:
                out.append("L_%X:;" % ea)
            out.append(text)
        out.append("  throw FormatError(\"fell off the end of 0x%X\");" % v["ea"])
        out.append("}")
        return "\n".join(out)


def main():
    insns_path, out_dir = sys.argv[1], sys.argv[2]
    funcs = json.load(open(insns_path))
    funcs = {n: v for n, v in funcs.items() if n not in SHIM_OVERRIDE}
    retn = {}
    if len(sys.argv) > 3:
        for line in open(sys.argv[3]):
            k, v = line.split()
            retn[k] = int(v)
    # address -> name for boundary calls
    shimnames = {}
    fmap = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "analysis", "retail_function_map.tsv")
    for line in open(fmap, encoding="utf-8").read().split("\n")[1:]:
        p = line.split("\t")
        if len(p) > 1:
            shimnames[int(p[0], 16)] = p[1]
    g = Gen(funcs, shimnames, retn)
    os.makedirs(out_dir, exist_ok=True)
    names = sorted(funcs, key=lambda n: funcs[n]["ea"])
    per = 60
    files = []
    # code addresses used as immediates (callbacks such as `mov edx, offset loc_220E0`) that land in the middle of a function need an entry point too
    owner = {}
    for n, v in funcs.items():
        for r in v["insns"]:
            owner.setdefault(r["ea"], n)
    for n, v in list(funcs.items()):
        for r in v["insns"]:
            if r["mn"] in ("mov", "push", "lea", "cmp"):
                for op in r["ops"]:
                    if op["t"] == 5 and op["val"] in owner and op["val"] not in g.entries and r["ea"] != op["val"]:
                        tgt = op["val"]
                        o = funcs[owner[tgt]]
                        funcs["tail_%X" % tgt] = {"ea": tgt, "insns": [x for x in o["insns"] if x["ea"] >= tgt]}
                        g.entries[tgt] = "tail_%X" % tgt
    names = sorted(funcs, key=lambda n: funcs[n]["ea"])
    texts = {}
    skipped = {}
    for n in names:
        try:
            texts[n] = g.function(n, funcs[n])
        except ValueError as e:
            skipped[n] = str(e)
    for n in skipped:                       # functions that cannot be translated stay shims (no-ops)
        g.entries.pop(funcs[n]["ea"], None)
        del funcs[n]
    while g.pending:
        t = g.pending.pop()
        texts[t] = g.function(t, funcs[t])
    names = sorted(funcs, key=lambda n: funcs[n]["ea"])
    for i in range(0, len(names), per):
        chunk = names[i:i + per]
        code = ['// GENERATED by reversing/scripts/transpile_handlers.py from SHANNARA.EXE - not committed', '#include "machine.h"', "namespace shannara {", ""]
        for n in chunk:
            code.append(texts[n])
            code.append("")
        code.append("void registerGen%d(Machine &m) {" % (i // per))
        for n in chunk:
            code.append("  m.registerFunction(0x%X, f_%X);" % (funcs[n]["ea"], funcs[n]["ea"]))
        code.append("}")
        code.append("} // namespace shannara")
        fn = "gen_%02d.cpp" % (i // per)
        open(os.path.join(out_dir, fn), "w").write("\n".join(code) + "\n")
        files.append(fn)
    reg = ['// GENERATED', '#include "machine.h"', "namespace shannara {"]
    for k in range(len(files)):
        reg.append("void registerGen%d(Machine &);" % k)
    reg.append("void registerAllGenerated(Machine &m) {")
    for k in range(len(files)):
        reg.append("  registerGen%d(m);" % k)
    for a, nm in sorted(g.boundary.items()):
        reg.append("  m.registerShim(0x%X, \"%s\", %d);" % (a, nm.replace("\\", ""), retn.get(nm, 0)))
    # shims also needed for overridden functions that handlers call
    for a, nm in sorted(shimnames.items()):
        if nm in SHIM_OVERRIDE and a not in g.boundary:
            reg.append("  m.registerShim(0x%X, \"%s\", %d);" % (a, nm, retn.get(nm, 0)))
    reg.append("}")
    reg.append("} // namespace shannara")
    open(os.path.join(out_dir, "gen_registry.cpp"), "w").write("\n".join(reg) + "\n")
    print("functions:", len(names), "files:", len(files), "boundary targets:", len(g.boundary), "skipped:", len(skipped))
    with open(os.path.join(out_dir, "skipped.txt"), "w") as f:
        for n, why in sorted(skipped.items()):
            f.write("%s: %s\n" % (n, why))


if __name__ == "__main__":
    main()
