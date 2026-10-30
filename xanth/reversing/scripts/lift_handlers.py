"""
x86-16 -> C++ lifter for the Xanth handlers that the digest-based translator (gen_handlers.py) cannot express (loops, es:[bx+..] accesses, tail jumps, jump tables).

It works on xanth.asm directly: every instruction reachable from a function's entry (following fall-through, jumps, conditional jumps, jump tables and tail jumps
into chunks that IDA placed inside other functions) is lowered to C++ operating on a small register/flag model (engine/gen/cpu.h) with labels and gotos.
Calls become `H_<name>(c)` (host functions that read their cdecl arguments from the emulated stack) or `L_<name>(c)` for functions lifted here.

    python lift_handlers.py symbols            # list symbol operands (for dump_symbols.py)
    python lift_handlers.py                    # write ../../engine/gen/lifted_handlers.cpp and lifted_api.h
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ASM = os.path.join(HERE, "..", "..", "xanth.asm")
GEN = os.path.join(HERE, "..", "..", "engine", "gen")

REG16 = {"ax", "bx", "cx", "dx"}
REG8 = {"al": ("ax", "l"), "ah": ("ax", "h"), "bl": ("bx", "l"), "bh": ("bx", "h"), "cl": ("cx", "l"), "ch": ("cx", "h"), "dl": ("dx", "l"), "dh": ("dx", "h")}
PLAIN16 = {"si", "di", "bp", "sp"}
SEGREG = {"es", "ds", "ss", "cs"}


class Insn:
    __slots__ = ("idx", "mn", "ops", "label", "text", "pfx", "proc")

    def __init__(self, idx, mn, ops, text, pfx=""):
        self.idx, self.mn, self.ops, self.text, self.pfx = idx, mn, ops, text, pfx
        self.label = []


# seg157:0402.. (xanth.asm byte_70402): the bytes 8C 5E F8 FF 76 F8 FF 76 F6 B8 03 00 BA 28 F0 52 50 9A 3B 47 <sg19DD> are code that IDA left as data;
# word_70405 (the second instruction) is a jump target too.  Decoded by hand: Font_WriteMessage(ds:[bp-0Ah], msg 0xF028:3).
# Each patch replaces data lines of xanth.asm that are really code: it starts at the first line matching `start`, drops every line up to and including the first
# line matching `until`, and inserts the hand-decoded instructions (bytes read from the IDB image) with their labels.  `procs` registers far functions among them.
PATCHES = [
    {   # seg157:0248.. : the tail of the function at loc_701CD (mov ax,1 / jmp / nop / xor ax,ax / retf) and the whole next function word_7024E
        "start": r"^word_70248\s+dw 3EBh", "until": r"^\s+retf\b", "procs": ["word_7024E"],
        "insns": [
            ([], "jmp loc_7024D_h"),
            (["word_7024A_p1", "word_7024A+1"], "xor ax, ax"),
            (["loc_7024D_h"], "retf"),
            (["word_7024E"], "mov ax, 0Eh"), ([], "push ax"), ([], "mov ax, 144h"), ([], "push ax"), ([], "call far ptr object_has_attr?"), ([], "add sp, 4"),
            ([], "or ax, ax"), ([], "jz loc_7027C_h"),
            ([], "mov ax, 1"), ([], "push ax"), ([], "mov ax, 145h"), ([], "push ax"), ([], "call far ptr object_get_list_head?"), ([], "add sp, 4"),
            ([], "or ax, ax"), ([], "jz loc_7027C_h"),
            ([], "mov ax, 1"), ([], "jmp loc_7027E_h"),
            (["loc_7027C_h"], "xor ax, ax"),
            (["loc_7027E_h"], "retf"),
        ]},
    {   # seg157:0402..: 8C 5E F8 FF 76 F8 FF 76 F6 B8 03 00 BA 28 F0 52 50 9A <Font_WriteMessage>; word_70405 is a jump target too
        "start": r"^byte_70402\s+db", "until": r"dw seg sg19DD", "procs": [],
        "insns": [
            (["byte_70402"], "mov word ptr [bp-8], ds"),
            (["word_70405"], "push word ptr [bp-8]"),
            ([], "push word ptr [bp-0Ah]"), ([], "mov ax, 3"), ([], "mov dx, 0F028h"), ([], "push dx"), ([], "push ax"), ([], "call Font_WriteMessage"),
        ]},
]


DATAREFS = set()   # names stored in dd/dw data (function tables)
SYMKIND = {}    # data symbol -> db / dw / dd (as defined in the listing)


# Handlers whose code IDA left as plain code chunks (no function, so apply_xanth_handlers.py could not name them): handler number -> (label, linear address).
# Derived from the RTLink stubs in object_handler_table (stub = E8 rel16 / EA off16 seg16; the far jump target is the label).
CHUNK_HANDLERS = {1: ("loc_1E817", 0x1E817), 27: ("loc_5CA3F", 0x5CA3F), 30: ("loc_5CB83", 0x5CB83), 45: ("loc_4DD8C", 0x4DD8C), 86: ("loc_51085", 0x51085),
                  177: ("loc_5B156", 0x5B156), 180: ("loc_5B25A", 0x5B25A), 335: ("loc_7480B", 0x7480B)}
EXTRA_FUNCS = ["sub_540A4"]     # target of a function-table stub that IDA did not recognise as a stub


def load_asm():
    """-> (insns, labels{name: insn index}, procs{name: insn index}, tables{name: [labels]}, locals{proc: {name: (size, offset)}})"""
    insns, labels, procs, tables, locs = [], {}, {}, {}, {}
    cur_proc, cur_tab, pending = None, None, []
    patch = None                        # patch being applied (skipping the data lines it replaces)
    for line in open(ASM, encoding="utf-8", errors="replace"):
        line = line.rstrip("\n")
        if patch:
            if re.search(patch["until"], line):
                patch = None
            continue
        for p in PATCHES:
            if re.match(p["start"], line):
                for labs, text in p["insns"]:
                    parts = text.split(None, 1)
                    ops = [o.strip() for o in parts[1].split(",")] if len(parts) > 1 else []
                    i = Insn(len(insns), parts[0], ops, text)
                    i.label, i.proc = pending + list(labs), cur_proc
                    for lab in i.label:
                        labels[lab] = i.idx
                        if lab in p["procs"]:
                            procs[lab] = i.idx
                    pending = []
                    insns.append(i)
                patch = p
                break
        if patch:
            if re.search(patch["until"], line):      # start and end on the same line
                patch = None
            continue
        m = re.match(r"^(\S+)\s+proc (near|far)", line)
        if m:
            cur_proc = m.group(1)
            locs[cur_proc] = {}
            procs[cur_proc] = len(insns)
            pending.append(cur_proc)
            cur_tab = None
            continue
        if cur_proc and re.match(r"^%s\s+endp" % re.escape(cur_proc), line):
            cur_proc = None
            continue
        mt = re.match(r"^\s*(?:[\w?]+\s+)?(?:dd|dw)\s+(?:offset\s+)?([A-Za-z_][\w?]*)\s*(?:;.*)?$", line)
        if mt:
            DATAREFS.add(mt.group(1))
        md = re.match(r"^([\w?]+)\s+(db|dw|dd)", line)
        if md:
            SYMKIND.setdefault(md.group(1), md.group(2))
        m = re.match(r"^(\w+)\s+dw offset (\w+)", line)
        if m:
            cur_tab = tables.setdefault(m.group(1), [])
            cur_tab.append(m.group(2))
            continue
        m = re.match(r"^\s+dw offset (\w+)", line)
        if m and cur_tab is not None:
            cur_tab.append(m.group(1))
            continue
        code = line.split(";")[0].rstrip()
        if not code.strip():
            continue
        m = re.match(r"^(\w+)\s*=\s*(byte|word|dword) ptr\s+(-?[0-9A-Fa-f]+h?)\s*$", code.strip())
        if m and cur_proc:
            locs[cur_proc][m.group(1)] = ({"byte": 1, "word": 2, "dword": 4}[m.group(2)], num(m.group(3)))
            continue
        m = re.match(r"^(\w+):\s*$", code.strip()) if not code.startswith((" ", "\t")) else None
        if m:
            pending.append(m.group(1))
            labels[m.group(1)] = len(insns)
            continue
        if not code.startswith((" ", "\t")):
            continue
        s = code.strip()
        if re.match(r"^(assume|align|db|dw|dd|public|org|ends|segment|;)", s) or re.match(r"^\w+\s*(=|equ)\s", s):
            cur_tab = None if not s.startswith(("align", "assume")) else cur_tab
            continue
        cur_tab = None
        pfx = ""
        mm = re.match(r"^(rep|repe|repne|repz|repnz|lock)\s+(.*)$", s)
        if mm:
            pfx, s = mm.group(1), mm.group(2)
        parts = s.split(None, 1)
        mn = parts[0].lower()
        ops = [o.strip() for o in parts[1].split(",")] if len(parts) > 1 else []
        i = Insn(len(insns), mn, ops, s, pfx)
        i.label = pending
        i.proc = cur_proc
        for p in pending:
            labels[p] = i.idx
            if p in procs:
                procs[p] = i.idx
        pending = []
        insns.append(i)
    for num_, (lab, ea) in CHUNK_HANDLERS.items():
        if lab in labels:
            procs.setdefault("objh_%03d" % num_, labels[lab])
    if "loc_701CD" in labels:                 # a function whose first instructions IDA did not turn into a function
        procs.setdefault("loc_701CD", labels["loc_701CD"])
    return insns, labels, procs, tables, locs


NUM = re.compile(r"^-?[0-9][0-9A-Fa-f]*h?$")


def num(tok):
    tok = tok.strip()
    neg = tok.startswith("-")
    t = tok.lstrip("-")
    v = int(t[:-1], 16) if t.lower().endswith("h") else int(t)
    return -v if neg else v


def is_num(tok):
    return bool(NUM.match(tok.strip())) and (tok.strip().lower().endswith("h") or tok.strip().lstrip("-").isdigit())


def reachable(insns, labels, tables, entry):
    seen, stack = set(), [entry]
    while stack:
        i = stack.pop()
        if i in seen or i >= len(insns):
            continue
        seen.add(i)
        x = insns[i]
        mn = x.mn
        tgt = None
        if mn.startswith("j") or mn in ("loop", "loope", "loopne", "jcxz"):
            op = re.sub(r"^((short|near ptr|far ptr|near|far)\s+)+", "", x.ops[0].strip()) if x.ops else ""
            m = re.match(r"^cs:(\w+)\[bx\]$", op)
            if m and m.group(1) in tables:
                for lab in tables[m.group(1)]:
                    if lab in labels:
                        stack.append(labels[lab])
                continue
            if op in labels:
                stack.append(labels[op])
            if mn == "jmp":
                continue
        elif mn in ("retf", "retn", "ret", "iret"):
            continue
        stack.append(i + 1)
    return sorted(seen)


SYMTOK = re.compile(r"[A-Za-z_][\w?.$@]*")
SKIP = REG16 | set(REG8) | PLAIN16 | SEGREG | {"word", "byte", "dword", "ptr", "offset", "short", "near", "far", "seg"}


def operand_symbols(ins, labels, locs):
    """data symbols (not registers, locals, labels or numbers) used by an instruction's operands"""
    out = set()
    if ins.mn.startswith("j") or ins.mn in ("call", "loop", "loope", "loopne"):
        ops = [o for o in ins.ops if not re.match(r"^(short |near |far ptr )?[\w?.$@]+$", o)] if ins.mn == "call" else []
    else:
        ops = ins.ops
    for op in ops:
        for t in SYMTOK.findall(op):
            if t.lower() in SKIP or is_num(t) or re.match(r"^[0-9A-F]+h$", t):
                continue
            base = t.split(".")[0]
            if t in locs.get(ins.proc, {}) or base in labels and base.startswith("loc_"):
                continue
            out.add(base)
    return out


HOSTMODE = "--host" in sys.argv
if HOSTMODE:
    sys.argv.remove("--host")
    GEN = os.path.join(HERE, "..", "..", "engine", "host", "gen")      # full lift for the host build (git-ignored)
    os.makedirs(GEN, exist_ok=True)
ALL = "--all" in sys.argv or HOSTMODE
if "--all" in sys.argv:
    sys.argv.remove("--all")
    GEN = os.path.join(HERE, "..", "..", "engine", "gen_all")      # test build only (not committed)
    os.makedirs(GEN, exist_ok=True)


def todo_names():
    if ALL:
        out = []
        for l in open(os.path.join(HERE, "..", "analysis", "xanth_handler_digest.txt"), encoding="utf-8"):
            if l and not l.startswith(" ") and re.match(r"^(objh_\d+|exit_fn_\d+|event_fn_\d+|conv_cb_\d+|conv_text_\d+)", l):
                out.append(l.split()[0])
        return out
    return [l.split("	")[0] for l in open(os.path.join(HERE, "..", "..", "engine", "gen", "handlers_todo.txt")) if l.strip()]


# ------------------------------------------------------------------------------------------------------------------------------------------------------
# lowering

COND = {
    "jz": "c.ZF", "je": "c.ZF", "jnz": "!c.ZF", "jne": "!c.ZF",
    "jb": "c.CF", "jc": "c.CF", "jnae": "c.CF", "jnb": "!c.CF", "jnc": "!c.CF", "jae": "!c.CF",
    "jbe": "(c.CF || c.ZF)", "jna": "(c.CF || c.ZF)", "ja": "(!c.CF && !c.ZF)", "jnbe": "(!c.CF && !c.ZF)",
    "jl": "(c.SF != c.OF)", "jnge": "(c.SF != c.OF)", "jge": "(c.SF == c.OF)", "jnl": "(c.SF == c.OF)",
    "jle": "(c.ZF || c.SF != c.OF)", "jng": "(c.ZF || c.SF != c.OF)", "jg": "(!c.ZF && c.SF == c.OF)", "jnle": "(!c.ZF && c.SF == c.OF)",
    "js": "c.SF", "jns": "!c.SF", "jo": "c.OF", "jno": "!c.OF", "jp": "false", "jnp": "true",
}
UNRESOLVED = []
HOSTS = set()


def cid(name):
    return re.sub(r"[^A-Za-z0-9_]", "_", name.replace("?", "_q"))


def regx(name):
    return "c.%s.x" % name if name in REG16 else "c.%s" % name


class Lift:
    def __init__(self, insns, labels, procs, tables, locs, syms, segs, lifted):
        self.insns, self.labels, self.procs, self.tables, self.locs = insns, labels, procs, tables, locs
        self.syms, self.segs, self.lifted = syms, segs, lifted
        self.entry = None
        self.used_syms = set()
        self.used_segs = set()
        self.procfar = {}
        self.stubs = {}
        for pname, pidx in procs.items():
            a = insns[pidx:pidx + 2]
            if len(a) == 2 and a[0].mn == "call" and a[0].ops[0].split()[-1] == "rtlink_thunk" and a[1].mn == "jmp" and not a[1].ops[0].startswith("cs:"):
                self.stubs[pname] = re.sub(r"^((short|near ptr|far ptr|near|far)\s+)+", "", a[1].ops[0].strip())

    # --- operands -------------------------------------------------------------------------------------------------------------------------------
    def local(self, name, ins):
        for p in (ins.proc, self.entry):
            if p in self.locs and name in self.locs[p]:
                return self.locs[p][name]
        return None

    def symoff(self, name):
        """C++ expression for a data symbol's offset within its segment"""
        base, _, field = name.partition(".")
        s = self.syms.get(base)
        mf = re.match(r"^field_([0-9A-Fa-f]+)$", field)
        if mf and (not s or s.get("kind") != "data"):
            return "0x%s" % mf.group(1)          # `StructType.field_N`: IDA's struct-offset form of the constant N
        if not s or s.get("kind") != "data":
            UNRESOLVED.append(name)
            return "0 /*unresolved %s*/" % name
        self.used_syms.add(base)
        e = "D_%s" % cid(base)
        if field:
            m = re.match(r"^field_([0-9A-Fa-f]+)$", field)
            if m:
                e += "+0x%s" % m.group(1)
            else:
                UNRESOLVED.append(name)
                e += "/*field %s?*/" % field
        return e

    def symsize(self, name):
        base = name.split(".")[0]
        s = self.syms.get(base)
        if not s:
            return None
        if base.startswith("byte_"):
            return 1
        if base.startswith("word_"):
            return 2
        if base.startswith("dword_"):
            return 4
        return s["size"] if s.get("size") in (1, 2) else None

    def classify(self, op, ins):
        """-> dict(kind=reg|reg8|seg|imm|mem|offset|segof, ...)"""
        o = op.strip()
        size = None
        m = re.match(r"^(byte|word|dword|qword)\s+ptr\s+(.*)$", o)
        if m:
            size = {"byte": 1, "word": 2, "dword": 4, "qword": 8}[m.group(1)]
            o = m.group(2).strip()
        o = re.sub(r"^(short|near ptr|far ptr)\s+", "", o)
        if o in REG16 or o in PLAIN16:
            return dict(kind="reg", name=o, size=2)
        if o in REG8:
            return dict(kind="reg8", name=o, size=1)
        if o in SEGREG:
            return dict(kind="seg", name=o, size=2)
        if o.startswith("(") and o.endswith(")") and "offset" in o:
            o = o[1:-1].strip()
        m = re.match(r"^offset\s+(.*)$", o)
        if m:
            mm = re.match(r"^([\w?.$@]+)(?:\s*([+-])\s*(\w+))?$", m.group(1).strip())
            off = 0
            if mm.group(2):
                off = num(mm.group(3)) * (1 if mm.group(2) == "+" else -1)
            return dict(kind="offset", sym=mm.group(1), add=off, size=2)
        m = re.match(r"^seg\s+([\w?.$@]+)$", o)
        if m:
            return dict(kind="segof", sym=m.group(1), size=2)
        if is_num(o):
            return dict(kind="imm", value=num(o), size=size)
        if re.match(r"^'.'$", o):
            return dict(kind="imm", value=ord(o[1]), size=size)
        # memory operand
        seg = None
        m = re.match(r"^(es|ds|cs|ss):(.*)$", o)
        if m:
            seg, o = m.group(1), m.group(2)
        e = o.replace("[", "+").replace("]", "+").replace("(", "+").replace(")", "+")
        terms = re.findall(r"([+-]?)\s*([^+\-\s]+)", e)
        regs, const, syms_ = [], 0, []
        for sign, t in terms:
            sg = -1 if sign == "-" else 1
            if t in ("bx", "bp", "si", "di"):
                regs.append(t)
            elif is_num(t) or re.match(r"^[0-9][0-9A-Fa-f]*h$", t):
                const += sg * num(t)
            else:
                lv = self.local(t, ins)
                if lv:
                    const += sg * lv[1]
                    if size is None and lv[0] in (1, 2, 4):
                        size = lv[0]
                    if "bp" not in regs:
                        regs.append("bp")
                else:
                    syms_.append((sg, t))
        if seg is None:
            seg = "ss" if "bp" in regs else "ds"
        parts = [regx(r) for r in regs]
        for sg, t in syms_:
            parts.append(("-" if sg < 0 else "") + "(" + self.symoff(t) + ")")
            if size is None:
                size = self.symsize(t)
        if const:
            parts.append(str(const))
        off = "(uint16_t)(%s)" % (" + ".join(parts) if parts else "0")
        return dict(kind="mem", seg="c.%s" % seg, off=off, size=size)

    def rd(self, d, size):
        k = d["kind"]
        if k == "reg":
            return regx(d["name"])
        if k == "reg8":
            r, h = REG8[d["name"]]
            return "c.%s.%s" % (r, h)
        if k == "seg":
            return "c.%s" % d["name"]
        if k == "imm":
            v = d["value"]
            return "(uint%d_t)%d" % (8 * (size or 2), v) if v < 0 else "%d" % v
        if k == "offset":
            s = self.syms.get(d["sym"])
            if s and s.get("kind") == "data":
                return "(uint16_t)(%s%+d)" % (self.symoff(d["sym"]), d["add"])
            UNRESOLVED.append("offset " + d["sym"])
            return "0 /*offset %s*/" % d["sym"]
        if k == "segof":
            s = self.syms.get(d["sym"])
            if (s and s.get("kind") == "seg") or d["sym"] in self.segs or (s and s.get("kind") == "data"):
                self.used_segs.add(d["sym"])
                return "SEG_%s" % cid(d["sym"])
            UNRESOLVED.append("seg " + d["sym"])
            return "0 /*seg %s*/" % d["sym"]
        if k == "mem":
            return "c.rd%d(%s, %s)" % (8 * (size or 2), d["seg"], d["off"])
        raise ValueError(k)

    def wr(self, d, size, val):
        k = d["kind"]
        if k in ("reg", "reg8", "seg"):
            return "%s = %s;" % (self.rd(d, size), val)
        if k == "mem":
            return "c.wr%d(%s, %s, %s);" % (8 * (size or 2), d["seg"], d["off"], val)
        raise ValueError("cannot write " + k)

    def rmw(self, d, size, fn):
        """read-modify-write; fn(old expr) -> new expr"""
        if d["kind"] == "mem":
            return "{ uint16_t _s = %s, _o = %s; c.wr%d(_s, _o, %s); }" % (d["seg"], d["off"], 8 * size, fn("c.rd%d(_s, _o)" % (8 * size)))
        return "%s = %s;" % (self.rd(d, size), fn(self.rd(d, size)))

    @staticmethod
    def size_of(ds_):
        for d in ds_:
            if d["kind"] in ("reg", "seg"):
                return 2
            if d["kind"] == "reg8":
                return 1
        for d in ds_:
            if d.get("size") in (1, 2):
                return d["size"]
        return 2

    # --- instruction lowering ------------------------------------------------------------------------------------------------------------------
    def lower(self, x, out):
        mn, ops = x.mn, x.ops
        flow = mn in ("call", "jmp", "jcxz", "loop", "loope", "loopne") or mn in COND
        cl = [] if flow and mn != "call" else [self.classify(o, x) for o in ops] if mn != "call" else []
        T = lambda sz: "uint%d_t" % (8 * sz)
        emit = out.append

        if mn in ("nop", "cli", "sti", "wait"):
            return
        if mn == "cld":
            emit("c.DF = false;")
        elif mn == "std":
            emit("c.DF = true;")
        elif mn == "clc":
            emit("c.CF = false;")
        elif mn == "stc":
            emit("c.CF = true;")
        elif mn == "cmc":
            emit("c.CF = !c.CF;")
        elif mn in ("pushf", "popf", "pushfd", "popfd"):
            emit("// %s (flags are not saved)" % mn)
        elif mn == "mov":
            sz = self.size_of(cl)
            emit(self.wr(cl[0], sz, self.rd(cl[1], sz)))
        elif mn == "xchg":
            sz = self.size_of(cl)
            emit("{ %s _t = %s; %s %s }" % (T(sz), self.rd(cl[0], sz), self.wr(cl[0], sz, self.rd(cl[1], sz)), self.wr(cl[1], sz, "_t")))
        elif mn == "push":
            emit("c.push(%s);" % self.rd(cl[0], 2))
        elif mn == "pop":
            emit(self.wr(cl[0], 2, "c.pop()"))
        elif mn in ("add", "sub", "adc", "sbb", "and", "or", "xor"):
            sz = self.size_of(cl)
            t = T(sz)
            src = self.rd(cl[1], sz)
            if cl[1]["kind"] == "imm":
                src = "(%s)%d" % (t, cl[1]["value"])
            f = {"add": "opAdd<%s>(c, {0}, {1})", "sub": "opSub<%s>(c, {0}, {1})", "adc": "opAdd<%s>(c, {0}, {1}, c.CF)", "sbb": "opSub<%s>(c, {0}, {1}, c.CF)",
                 "and": "opLogic<%s>(c, (%s)(({0}) & ({1})))", "or": "opLogic<%s>(c, (%s)(({0}) | ({1})))", "xor": "opLogic<%s>(c, (%s)(({0}) ^ ({1})))"}[mn]
            f = f.replace("%s", t)
            emit(self.rmw(cl[0], sz, lambda old: f.replace("{0}", old).replace("{1}", src)))
        elif mn in ("cmp", "test"):
            sz = self.size_of(cl)
            t = T(sz)
            a, b = self.rd(cl[0], sz), self.rd(cl[1], sz)
            if cl[1]["kind"] == "imm":
                b = "(%s)%d" % (t, cl[1]["value"])
            emit(("(void)opSub<%s>(c, %s, %s);" % (t, a, b)) if mn == "cmp" else ("(void)opLogic<%s>(c, (%s)((%s) & (%s)));" % (t, t, a, b)))
        elif mn in ("inc", "dec", "neg", "not"):
            sz = self.size_of(cl)
            t = T(sz)
            f = {"inc": "opInc<%s>(c, {0})", "dec": "opDec<%s>(c, {0})", "neg": "opNeg<%s>(c, {0})", "not": "(%s)~({0})"}[mn].replace("%s", t)
            emit(self.rmw(cl[0], sz, lambda old: f.replace("{0}", old)))
        elif mn in ("shl", "sal", "shr", "sar", "rol", "ror"):
            sz = self.size_of(cl[:1])
            t = T(sz)
            cnt = "c.cx.l" if ops[1].strip() == "cl" else (str(num(ops[1])) if is_num(ops[1]) else "1")
            fn = {"shl": "opShl", "sal": "opShl", "shr": "opShr", "sar": "opSar", "rol": "opRol", "ror": "opRor"}[mn]
            emit(self.rmw(cl[0], sz, lambda old: "%s<%s>(c, %s, %s)" % (fn, t, old, cnt)))
        elif mn == "lea":
            emit("%s = %s;" % (self.rd(cl[0], 2), cl[1]["off"]))
        elif mn in ("les", "lds"):
            emit("{ uint16_t _s = %s, _o = %s; %s = c.rd16(_s, _o); c.%s = c.rd16(_s, (uint16_t)(_o + 2)); }" % (cl[1]["seg"], cl[1]["off"], self.rd(cl[0], 2), mn[1:]))
        elif mn == "cbw":
            emit("c.ax.x = (uint16_t)(int16_t)(int8_t)c.ax.l;")
        elif mn == "cwd":
            emit("c.dx.x = (c.ax.x & 0x8000) ? 0xFFFF : 0;")
        elif mn in ("mul", "imul", "div", "idiv") and len(ops) == 1:
            sz = self.size_of(cl)
            v = self.rd(cl[0], sz)
            if mn in ("mul", "imul"):
                if sz == 1:
                    e = "(int16_t)((int8_t)c.ax.l * (int8_t)(%s))" % v if mn == "imul" else "(uint16_t)(c.ax.l * (%s))" % v
                    emit("{ c.ax.x = (uint16_t)%s; c.CF = c.OF = c.ax.h != 0; }" % e)
                else:
                    e = "(uint32_t)((int32_t)(int16_t)c.ax.x * (int16_t)(%s))" % v if mn == "imul" else "(uint32_t)c.ax.x * (%s)" % v
                    emit("{ uint32_t _r = %s; c.ax.x = (uint16_t)_r; c.dx.x = (uint16_t)(_r >> 16); c.CF = c.OF = c.dx.x != 0; }" % e)
            elif sz == 1:
                sg = mn == "idiv"
                emit("{ int _d = %s(%s); if (_d) { int _a = %s; c.ax.l = (uint8_t)(_a / _d); c.ax.h = (uint8_t)(_a %% _d); } }" % ("(int8_t)" if sg else "(uint8_t)", v, "(int16_t)c.ax.x" if sg else "c.ax.x"))
            elif mn == "idiv":
                emit("{ int32_t _d = (int16_t)(%s); if (_d) { int32_t _a = (int32_t)(((uint32_t)c.dx.x << 16) | c.ax.x); c.ax.x = (uint16_t)(_a / _d); c.dx.x = (uint16_t)(_a %% _d); } }" % v)
            else:
                emit("{ uint32_t _d = (uint16_t)(%s); if (_d) { uint32_t _a = ((uint32_t)c.dx.x << 16) | c.ax.x; c.ax.x = (uint16_t)(_a / _d); c.dx.x = (uint16_t)(_a %% _d); } }" % v)
        elif mn == "imul":
            third = self.rd(cl[2], 2) if len(ops) == 3 else self.rd(cl[0], 2)
            emit("%s = (uint16_t)((int16_t)(%s) * (int16_t)(%s));" % (self.rd(cl[0], 2), self.rd(cl[1], 2), third))
        elif mn in COND or mn in ("jcxz", "loop", "loope", "loopne", "jmp"):
            self.lower_jump(x, cl, out)
        elif mn == "call":
            self.lower_call(x, cl, out)
        elif mn in ("retf", "retn", "ret", "iret"):
            emit("return;")
        elif mn in ("movsb", "movsw", "stosb", "stosw", "lodsb", "lodsw", "scasb", "scasw", "cmpsb", "cmpsw"):
            self.lower_string(x, out)
        else:
            emit("unimpl(\"%s\");   // not lifted: %s" % (mn, x.text.replace("*/", "")))
            UNRESOLVED.append("insn " + mn)

    def lower_jump(self, x, cl, out):
        mn, op = x.mn, re.sub(r"^((short|near ptr|far ptr|near|far)\s+)+", "", x.ops[0].strip())
        if mn == "jmp":
            m = re.match(r"^cs:([\w?]+)\[bx\]$", op)
            if m and m.group(1) in self.tables:
                out.append("switch (c.bx.x) {")
                for k, lab in enumerate(self.tables[m.group(1)]):
                    out.append("case %d: goto %s;" % (2 * k, cid(lab)))
                out.append("default: return;")
                out.append("}")
                return
            if op in self.labels and op not in self.procs:
                out.append("goto %s;" % cid(op))
                return
            self.tail(op, out)
            return
        if mn == "jcxz":
            cond = "c.cx.x == 0"
        elif mn.startswith("loop"):
            cond = "--c.cx.x != 0" + (" && c.ZF" if mn == "loope" else " && !c.ZF" if mn == "loopne" else "")
        else:
            cond = COND[mn]
        if op in self.labels and op not in self.procs:
            out.append("if (%s) goto %s;" % (cond, cid(op)))
        else:
            out.append("if (%s) {" % cond)
            self.tail(op, out)
            out.append("}")

    def tail(self, name, out):
        """jump to another function (tail call): call it, then return"""
        if name in self.procs or re.match(r"^[\w?.$@]+$", name):
            HOSTS.add(self.stubs.get(name, name))
            out.append(self.call_text(name) + " return;")
        else:
            out.append("unimpl(\"indirect jmp\"); return;   // %s" % name)
            UNRESOLVED.append("jmp " + name)

    def call_text(self, name):
        name = self.stubs.get(name, name)
        words = 1 if self.procfar.get(name) is False else 2
        fn = ("L_%s" if name in self.lifted else "H_%s") % cid(name)
        return "{ uint16_t _sp = c.sp; int _rw = c.retWords; c.retWords = %d; for (int _i = 0; _i < %d; _i++) c.push(0); if (c.onCall) c.onCall(c, \"%s\"); %s(c); c.sp = _sp; c.retWords = _rw; }" % (words, words, name.replace("\"", ""), fn)

    def lower_call(self, x, cl, out):
        op = x.ops[0].strip()
        t = re.sub(r"^((short|near ptr|far ptr|near|far)\s+)+", "", op)
        if t == "rtlink_thunk":                # overlay loader hook of an RTLink stub: nothing to do
            return
        if re.match(r"^[\w?.$@]+$", t) and t not in REG16 and t not in PLAIN16 and self.local(t, x) is None and (t in self.procs or t not in self.syms):
            HOSTS.add(self.stubs.get(t, t))
            out.append(self.call_text(t))
            return
        d = self.classify(op, x)
        farsym = any(SYMKIND.get(tok.split(".")[0]) == "dd" for tok in re.findall(r"[A-Za-z_][\w?.$@]*", op))
        if d["kind"] == "mem" and (d["size"] == 4 or "dword" in op or farsym):
            out.append("{ uint16_t _s = %s, _o = %s; uint16_t _off = c.rd16(_s, _o), _seg = c.rd16(_s, (uint16_t)(_o + 2)); uint16_t _sp = c.sp; int _rw = c.retWords; c.retWords = 2; c.push(0); c.push(0); if (c.callFarHook) c.callFarHook(c, _seg, _off); c.sp = _sp; c.retWords = _rw; }" % (d["seg"], d["off"]))
        else:
            out.append("{ uint16_t _off = %s; uint16_t _sp = c.sp; int _rw = c.retWords; c.retWords = 1; c.push(0); if (c.callFarHook) c.callFarHook(c, c.cs, _off); c.sp = _sp; c.retWords = _rw; }" % self.rd(d, 2))

    def lower_string(self, x, out):
        mn, pfx = x.mn, x.pfx
        w = 1 if mn.endswith("b") else 2
        step = "(c.DF ? -%d : %d)" % (w, w)
        base = mn[:-1]
        acc = "c.ax.l" if w == 1 else "c.ax.x"
        if base == "movs":
            body = "c.wr%d(c.es, c.di, c.rd%d(c.ds, c.si)); c.si += %s; c.di += %s;" % (8 * w, 8 * w, step, step)
        elif base == "stos":
            body = "c.wr%d(c.es, c.di, %s); c.di += %s;" % (8 * w, acc, step)
        elif base == "lods":
            body = "%s = c.rd%d(c.ds, c.si); c.si += %s;" % (acc, 8 * w, step)
        elif base == "scas":
            body = "(void)opSub<uint%d_t>(c, %s, c.rd%d(c.es, c.di)); c.di += %s;" % (8 * w, acc, 8 * w, step)
        else:
            body = "(void)opSub<uint%d_t>(c, c.rd%d(c.ds, c.si), c.rd%d(c.es, c.di)); c.si += %s; c.di += %s;" % (8 * w, 8 * w, 8 * w, step, step)
        if not pfx:
            out.append("{ %s }" % body)
        else:
            stop = ""
            if base in ("scas", "cmps"):
                stop = "if (c.ZF) break;" if pfx in ("repne", "repnz") else "if (!c.ZF) break;"
            out.append("while (c.cx.x) { %s c.cx.x--; %s }" % (body, stop))


WP = "LW_" if ALL else ""


SUPPORTED = {"mov", "push", "pop", "add", "sub", "adc", "sbb", "and", "or", "xor", "cmp", "test", "inc", "dec", "neg", "not", "shl", "sal", "shr", "sar", "rol", "ror",
             "lea", "les", "lds", "cbw", "cwd", "mul", "imul", "div", "idiv", "xchg", "call", "jmp", "retf", "retn", "ret", "cld", "std", "clc", "stc", "cmc", "nop",
             "cli", "sti", "pushf", "popf", "movsb", "movsw", "stosb", "stosw", "lodsb", "lodsw", "scasb", "scasw", "cmpsb", "cmpsw", "jcxz", "loop", "loope", "loopne"} | set(COND)
# engine functions that touch the screen, mouse, files, sound or memory manager: the host implements these (H_*) instead of lifting them
BLACKLIST = re.compile(r"^(Font_|Screen_|Mouse|Image_|Palette|File_|j_File|Events_|RoomItem_|Windows_|ResourceText|show_|wait_|popup_|realsound|play_|save_game|load_game|"
                       r"game_over|setup_pic|disable_pic|set_mouse|reset_color|Conversation_Show|wputchar|clear_text|memAlloc|Picture_|msleep|error_dialog|rtlink|Regions_|"
                       r"voc_|Music|set_room_music|update_animation|clear_keys|handle_|main$|game_init|load_pic|get_region|click_button|disable_button|build_command|"
                       r"interrupts|sub_27945|sub_29B35|showCutscene|Cutscene|sub_2A|sub_2B|sub_5F|sub_67E2A|sub_69FF7|sub_6C42D|_printf|_sprintf)")
MAXSIZE = 1300
STRIP = r"^((short|near ptr|far ptr|near|far)\s+)+"


def stub_map(insns, procs):
    out = {}
    for pname, pidx in procs.items():
        a = insns[pidx:pidx + 2]
        if len(a) == 2 and a[0].mn == "call" and a[0].ops[0].split()[-1] == "rtlink_thunk" and a[1].mn == "jmp" and not a[1].ops[0].startswith("cs:"):
            out[pname] = re.sub(STRIP, "", a[1].ops[0].strip())
    return out


def call_targets(insns, labels, procs, tables, name, stubs):
    out = set()
    for i in reachable(insns, labels, tables, procs[name]):
        x = insns[i]
        if x.mn == "call" or x.mn == "jmp" or x.mn in COND:
            op = re.sub(STRIP, "", x.ops[0].strip()) if x.ops else ""
            if op in procs:
                out.add(stubs.get(op, op))
        for o in x.ops:
            m = re.match(r"^(?:\()?offset\s+([A-Za-z_][\w?]*)", o.strip())
            if m and m.group(1) in procs:
                out.add(stubs.get(m.group(1), m.group(1)))
    return out


def liftable(name, insns, labels, procs, tables):
    if name not in procs or BLACKLIST.match(name):
        return False
    R = reachable(insns, labels, tables, procs[name])
    if len(R) > MAXSIZE or not all(insns[i].mn in SUPPORTED for i in R):
        return False
    # struct members with a name (GFX._bgColor) cannot be resolved to offsets from the asm text: leave such functions to the host
    return not any(re.search(r"[A-Za-z_]\w*\.(?!field_[0-9A-Fa-f]+)[A-Za-z_]", o) for i in R for o in insns[i].ops)


def closure(insns, labels, procs, tables, initial):
    """handlers + every engine function they (transitively) call that is plain game logic"""
    stubs = stub_map(insns, procs)
    out, seen, queue = list(initial), set(initial), list(initial)
    while queue:
        n = queue.pop(0)
        for t in sorted(call_targets(insns, labels, procs, tables, n, stubs)):
            if t not in seen:
                seen.add(t)
                if liftable(t, insns, labels, procs, tables):
                    out.append(t)
                    queue.append(t)
    return out


def lifted_set(insns, labels, procs, tables):
    names = todo_names()
    names = names + [n for n in ["objh_%03d" % k for k in CHUNK_HANDLERS] + EXTRA_FUNCS if n in procs and n not in names]
    if HOSTMODE:
        extra = [n for n in procs if re.match(r"^(cmd[A-Z]\w*|verbdef\w*)$", n)]
        stubs = stub_map(insns, procs)
        tabled = sorted({stubs.get(n, n) for n in DATAREFS if n in procs and liftable(stubs.get(n, n), insns, labels, procs, tables)})
        names = closure(insns, labels, procs, tables, names + [n for n in extra + tabled if n not in names])
    return names


def symbols_main():
    """write the data symbol names the lifted set uses (input for dump_symbols.py)"""
    insns, labels, procs, tables, locs = load_asm()
    syms = set()
    for n in lifted_set(insns, labels, procs, tables):
        for i in reachable(insns, labels, tables, procs[n]):
            syms |= operand_symbols(insns[i], labels, locs)
    json.dump(sorted(syms), open(sys.argv[2], "w"))
    print(len(syms), "symbols")


def main():
    insns, labels, procs, tables, locs = load_asm()
    syms_all = json.load(open(sys.argv[1]))
    syms, segs = syms_all["syms"], syms_all["segs"]
    funcs = json.load(open(sys.argv[2]))["funcs"] if len(sys.argv) > 2 else {}
    todo = lifted_set(insns, labels, procs, tables)
    lifted = set(todo)
    L = Lift(insns, labels, procs, tables, locs, syms, segs, lifted)
    for m in re.finditer(r"^(\S+)\s+proc (near|far)", open(ASM, encoding="utf-8", errors="replace").read(), re.M):
        L.procfar[m.group(1)] = m.group(2) == "far"
    body, count = [], 0
    for name in todo:
        L.entry = name
        R = reachable(insns, labels, tables, procs[name])
        count += len(R)
        lines = []
        for i in R:
            x = insns[i]
            for lab in x.label:
                if lab == name:
                    lines.append("entry: STEP();")
                elif lab not in procs:
                    lines.append("%s: STEP();" % cid(lab))
            lines.append("// %s" % x.text.replace("*/", "").replace("\\", "/"))
            L.lower(x, lines)
        lines.append(";")
        body.append("void L_%s(Cpu &c) {\n%s\n}\n" % (cid(name), "\n".join(l if l.endswith(":") else "\t" + l for l in lines)))
    dh = ["// generated by lift_handlers.py: data symbol offsets (within their segment) and segment paragraphs used by lifted_handlers.cpp", "#pragma once", "#include <cstdint>", ""]
    for s in sorted(L.used_syms):
        d = syms[s]
        dh.append("constexpr uint16_t D_%s = 0x%X;   // %s:%X (linear %X)" % (cid(s), d["ea"] - d["seg"] * 16, d["segname"], d["ea"] - d["seg"] * 16, d["ea"]))
    dh.append("")
    dh.append("constexpr uint16_t SEG_dseg_para = 0x%X;   // the default data segment (value of ds)" % segs.get("dseg", 0))
    for s in sorted(L.used_segs):
        para = syms[s]["para"] if s in syms and syms[s].get("kind") == "seg" else (syms[s]["seg"] if s in syms else segs.get(s, 0))
        dh.append("constexpr uint16_t SEG_%s = 0x%X;" % (cid(s), para))
    api = ["// generated by lift_handlers.py: host functions the lifted handlers call (read cdecl args with c.arg(n), return in ax / dx:ax)", "#pragma once", "#include \"cpu.h\"", ""]
    api += ["void H_%s(Cpu &c);" % cid(h) for h in sorted(HOSTS - lifted)]
    api += [""] + ["void L_%s(Cpu &c);" % cid(n) for n in todo]
    api += ["", "// handler-style entry points (same signature as the translated handlers in handlers_gen.cpp)"]
    api += ["intptr_t " + WP + "%s(int phase, int arg_2, int arg_4);" % cid(n) for n in todo]
    api += ["", "Cpu &liftedCpu();   // provided by the host"]
    if HOSTMODE:
        api += ["", "struct LiftedEntry { uint32_t linear; void (*fn)(Cpu &); const char *name; };", "extern const LiftedEntry kLifted[];", "extern const unsigned kLiftedCount;"]
    wrappers = []
    for n in todo:
        ret = "(intptr_t)(((uint32_t)c.dx.x << 16) | c.ax.x)" if n.startswith("conv_text") else "(int16_t)c.ax.x"
        wrappers.append("intptr_t " + WP + "%s(int phase, int arg_2, int arg_4) {\n\tCpu &c = liftedCpu();\n\tuint16_t sp0 = c.sp;\n\tc.push((uint16_t)arg_4);\n\tc.push((uint16_t)arg_2);\n\tc.push((uint16_t)phase);\n\tc.retWords = 2;\n\tc.push(0);\n\tc.push(0);\n\tL_%s(c);\n\tc.sp = sp0;\n\treturn %s;\n}\n" % (cid(n), cid(n), ret))
    open(os.path.join(GEN, "lifted_data.h"), "w").write("\n".join(dh) + "\n")
    open(os.path.join(GEN, "lifted_api.h"), "w").write("\n".join(api) + "\n")
    open(os.path.join(GEN, "lifted_handlers.cpp"), "w").write(
        "// generated by lift_handlers.py from xanth.asm - do not edit.  Each L_<name> is an instruction-level lowering of one handler (registers in Cpu, gotos for jumps).\n"
        "#include \"lifted_api.h\"\n#include \"lifted_data.h\"\n\n" + "\n".join(body) + "\n" + "\n".join(wrappers))
    if HOSTMODE:
        stubs = stub_map(insns, procs)
        ent = []
        for n in todo:
            if n in funcs:
                ent.append((funcs[n]["ea"], n))
        for num_, (lab, ea) in CHUNK_HANDLERS.items():
            if "objh_%03d" % num_ in lifted:
                ent.append((ea, "objh_%03d" % num_))
        for st, tgt in stubs.items():
            tgt2 = tgt
            while tgt2 in stubs:
                tgt2 = stubs[tgt2]
            if tgt2 in lifted and st in funcs:
                ent.append((funcs[st]["ea"], tgt2))
        ent = sorted(set(ent))
        open(os.path.join(GEN, "lifted_table.cpp"), "w").write(
            "// generated by lift_handlers.py: linear address (segment * 16 + offset) of every lifted function and of the RTLink stubs that jump to it\n"
            "#include \"lifted_api.h\"\nconst LiftedEntry kLifted[] = {\n" + "".join('\t{0x%X, L_%s, "%s"},\n' % (ea, cid(n), n) for ea, n in ent) + "};\nconst unsigned kLiftedCount = %d;\n" % len(ent))
        print("table entries", len(ent))
        hostset = sorted(HOSTS - lifted)
        open(os.path.join(GEN, "host_functions.txt"), "w").write("\n".join(cid(h) for h in hostset) + "\n")
        # name tables for the host: every IDB function (address, name, RTLink stub target) and the H_* implementations by original name
        rows = []
        for fname, f in sorted(funcs.items(), key=lambda kv: kv[1]["ea"]):
            tgt = stubs.get(fname, "")
            while tgt in stubs:
                tgt = stubs[tgt]
            rows.append('\t{0x%X, "%s", "%s"},\n' % (f["ea"], fname.replace('"', ""), tgt))
        open(os.path.join(GEN, "lifted_names.cpp"), "w").write(
            "// generated by lift_handlers.py\n#include \"lifted_api.h\"\nstruct FuncName { uint32_t linear; const char *name; const char *stubTarget; };\nextern const FuncName kFuncNames[] = {\n"
            + "".join(rows) + "};\nextern const unsigned kFuncNameCount = %d;\n" % len(rows)
            + "struct HostEntry { const char *name; void (*fn)(Cpu &); };\nextern const HostEntry kHosts[] = {\n"
            + "".join('\t{"%s", H_%s},\n' % (h, cid(h)) for h in hostset) + "};\nextern const unsigned kHostCount = %d;\n" % len(hostset))
        # weak default implementations (log and return 0) so the host only has to define the ones it implements
        open(os.path.join(GEN, "host_defaults.cpp"), "w").write(
            "// generated by lift_handlers.py: default (logging) bodies for every engine function the lifted code calls; the host overrides the ones it implements\n"
            "#include \"lifted_api.h\"\nvoid hostUnimplemented(const char *name, Cpu &c);\n"
            + "".join('__attribute__((weak)) void H_%s(Cpu &c) { hostUnimplemented("%s", c); }\n' % (cid(h), h) for h in hostset))
    print("lifted", len(todo), "functions;", count, "instructions;", len(HOSTS - lifted), "host functions;", len(set(UNRESOLVED)), "unresolved:", sorted(set(UNRESOLVED))[:30])


if __name__ == "__main__":
    if len(sys.argv) > 2 and sys.argv[1] == "symbols":
        symbols_main()
    elif len(sys.argv) >= 2:
        main()
