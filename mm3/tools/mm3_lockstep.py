#!/usr/bin/env python3
"""Lockstep comparison of the recompiled game against the original machine code.

    cd src && make mm3game_tape
    MM3_TAPE=build/run.tape SDL_VIDEODRIVER=dummy ./mm3game_tape ../data --headless --keys 4800,4800
    python3 ../tools/mm3_lockstep.py build/run.tape

The recorder (src/tape.c) logs every call the translated game code makes into a hand-written host routine: the name, the
arguments, the registers, and every change to memory in between.  This tool replays the tape against the *original* code
running in a CPU emulator (Unicorn), started from the same memory image and the same top-level routine:

  * a call the original code makes to a hand-written routine must be the next call on the tape, with the same arguments
    (otherwise: the translation or a hand-written routine took a different path -- the first difference is reported),
  * at every such call the data segment of the emulator must equal the data segment the recompiled game had,
  * "E" routines (DOS, files, video, sound, input ... things the original does through the machine) are replaced by the effect
    the recorded host had; "R" routines (readable rewrites, rules, C library) are *run* in the emulator, and their results and
    memory effects are compared to what the C replacement did.

It reports the first divergence (or "no divergence" and a coverage table).  Timing is not compared, only order, arguments and
state.
"""
import argparse
import os
import re
import struct
import sys

import capstone
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN, UcError
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
                               UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS,
                               UC_X86_REG_IP, UC_X86_REG_FLAGS, UC_X86_INS_IN, UC_X86_INS_OUT)

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import asm2c  # noqa: E402
from mm3_tape import Tape  # noqa: E402

DSEG = 0x286F
DG_LIN = DSEG * 16
SENT_SEG = 0x9F00                 # return address of the top-level call
# overlay stubs (int 3Fh thunks) and the overlays they lead to, as IDA has them
STUB_SEGMENTS = [0x27F2, 0x27FD, 0x2800, 0x2806, 0x280C, 0x2816, 0x2821, 0x282A, 0x2831, 0x2839, 0x2843, 0x2861, 0x2867]
OVERLAY_SEGMENTS = [0x378C, 0x3993, 0x3BC3, 0x3DD2, 0x3F49, 0x4150, 0x435A, 0x4579, 0x4790, 0x49B6, 0x4BD9, 0x4DE3, 0x4FF6]
CODE_SEGMENTS = sorted([0x1000, 0x14BE, 0x1523, 0x1903, 0x1B66, 0x1E40, 0x203F, 0x224F, 0x2511] + OVERLAY_SEGMENTS + STUB_SEGMENTS)

# Overlay 1 (378C:0) lies inside the 64 KB the recompiled game uses for DGROUP (the stack is at its top): the recompiled game never
# executes that memory, but the emulator does, so a copy of the overlay's code is placed at F000:0 (the BIOS ROM area, unused here) and used instead.
OVL1_SEG, OVL1_LOW_SEG = 0x378C, 0xF000

# routines run in the emulator and compared (everything else on the tape is replayed); value = number of result words
# C library helpers that are pure computation on memory: run in the emulator like the readable rewrites (value = result words)
LIBC_HOSTS = {"_memset": 1, "_memcpy_0": 2, "_strcpy": 1, "_strncpy_0": 2, "_stricmp": 1, "_sprintf": 1, "_atol": 2,
              "_memcpy": 1, "_memset_0": 2, "_ultoa": 1, "_sscanf": 1,
              "LXMUL@": 2, "LXLSH@": 2, "LXURSH@": 2, "F_LUDIV@": 2, "LUMOD@": 2, "SCOPY@": 0}
# Borland helpers that take their arguments in registers (compared at the call), and stack arguments the translator cannot count
REG_ARGS = {"LXMUL@": ("ax", "bx", "cx", "dx"), "LXLSH@": ("ax", "cx", "dx"), "LXURSH@": ("ax", "cx", "dx"), "SCOPY@": ("cx",)}
STACK_ARGS = {"F_LUDIV@": 4, "LUMOD@": 4, "SCOPY@": 4}
RULES_HOSTS = {"statBonus": 1, "getAge": 1, "itemScan": 1, "conditionMod": 1, "getStat": 1, "getCurrentLevel": 1,
               "getMaxHP": 1, "getMaxSP": 1, "getArmorClass": 1}


class Divergence(Exception):
    pass


class Aborted(Exception):
    """the recompiled game gave up on this routine call (watchdog): nothing to compare any more"""


def segment_of(linear):
    best = None
    for s in CODE_SEGMENTS:
        if s * 16 <= linear and (best is None or s > best):
            best = s
    return best


def canon(name):
    """tape / asm names: j_X (overlay thunk) is X; `F_LUDIV_AT` is the translator's spelling of F_LUDIV@"""
    if name.startswith("j_"):
        name = name[2:]
    return name.replace("_AT", "@") if name.endswith("_AT") else name


class Lockstep:
    def __init__(self, a):
        self.a = a
        self.tr = asm2c.Translator(a.asm, a.idc)
        self.tape = Tape(a.tape)
        self.evs = self.tape.events()
        self.peeked = []
        self.n_events = 0
        self.model = bytearray(self.tape.mem0)           # the recompiled game's memory, as the tape reports it
        self.uc = Uc(UC_ARCH_X86, UC_MODE_16)
        self.uc.mem_map(0, (self.tape.memsize + 0xFFF) & ~0xFFF)
        self.uc.mem_write(0, bytes(self.tape.mem0))
        stub = self.tape.mem0[STUB_SEGMENTS[0] * 16:STUB_SEGMENTS[0] * 16 + 16]
        self.ovl1_size = struct.unpack_from("<H", stub, 8)[0]
        img = open(a.image, "rb").read()
        self.uc.mem_write(OVL1_LOW_SEG * 16, img[OVL1_SEG * 16 - 0x10000:OVL1_SEG * 16 - 0x10000 + self.ovl1_size])
        self.limit = a.dg_limit                          # data segment bytes that are compared (the rest is the stack)
        self.ignore = self.parse_ignore(a.ignore)
        self.classes = self.load_classes(a)
        self.frames = []                                 # R routines currently running in the emulator
        self.stats = {}                                  # name -> [class, matched calls]
        self.recent = []
        self.host_since_label = False
        self.writes = {}                                 # replayed host routine -> data segment offsets it wrote
        self.fn_counts = {}
        self.last_heap = 0
        self.stop = None
        self.top_done = False
        self.setup_hooks()

    # ---------------------------------------------------------------- configuration
    def parse_ignore(self, items):
        out = []
        for it in items or []:
            lo, hi = it.split("-") if "-" in it else (it, it)
            out.append((int(lo, 16), int(hi, 16)))
        return out

    def load_classes(self, a):
        cls = {}
        ret_words = dict(RULES_HOSTS)
        diff_c = os.path.join(HERE, "..", "src", "game_diff.c")
        if os.path.exists(diff_c):
            for name, nargs, ret in re.findall(r'\{ "(\w+)", host_\w+, (\d+), (\d+),', open(diff_c).read()):
                ret_words[name] = int(ret)
        readable = os.path.join(HERE, "..", "src", "gen", "readable.txt")
        r_names = set(l.strip() for l in open(readable) if l.strip()) if os.path.exists(readable) else set()
        r_names |= set(RULES_HOSTS)
        r_names |= set(LIBC_HOSTS)
        ret_words.update(LIBC_HOSTS)
        r_names |= set(a.run or [])
        r_names -= set(a.replay or [])
        for n in r_names:
            cls[n] = ("R", ret_words.get(n, 0))
        return cls

    def klass(self, name):
        return self.classes.get(name, ("E", 0))

    # ---------------------------------------------------------------- tape access
    def peek(self):
        if not self.peeked:
            try:
                self.peeked.append(next(self.evs))
            except StopIteration:
                return None
        while self.peeked[0]["t"] == "L" and self.peeked[0]["name"] not in self.label_names:
            self.apply_delta(self.peeked.pop(0))          # (a label of another routine: only its memory changes matter)
            try:
                self.peeked.append(next(self.evs))
            except StopIteration:
                return None
        if self.peeked[0]["t"] == "X":
            self.peeked.pop(0)
            raise Aborted()
        return self.peeked[0]

    def take(self):
        ev = self.peek()
        if ev is None:
            raise StopIteration
        self.peeked.pop(0)
        self.n_events += 1
        return ev

    def apply_delta(self, ev, to_uc=False):
        for off, data in ev["delta"]:
            self.model[off:off + len(data)] = data
            if to_uc:
                self.uc.mem_write(off, data)

    # ---------------------------------------------------------------- hooks
    def code(self, lin):
        """where the emulator finds the code that IDA has at linear address `lin` (overlay 1 was moved, see OVL1_SEG)"""
        base = OVL1_SEG * 16
        return lin - base + OVL1_LOW_SEG * 16 if base <= lin < base + self.ovl1_size else lin

    def uncode(self, lin):
        base = OVL1_LOW_SEG * 16
        return lin - base + OVL1_SEG * 16 if base <= lin < base + self.ovl1_size else lin

    def func_extent(self, name):
        lin = self.tr.func_linear(name)
        if lin is None:
            return None
        starts = sorted(set(a for a in (self.tr.func_linear(n) for n in self.tr.procs) if a is not None))
        nxt = [s for s in starts if s > lin]
        return lin, (nxt[0] if nxt else lin + 0x400)

    def ret_sites(self, name, depth=0):
        """linear addresses of the ret / retf instructions of a routine (and of the routines it tail-jumps to)"""
        ext = self.func_extent(name)
        if not ext:
            return set()
        lin, end = ext
        end = min(end, lin + 0x4000)
        code = bytes(self.tape.mem0[lin:end])
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
        sites = set()
        off = 0
        while off < len(code):
            got = False
            for ins in md.disasm(code[off:], lin + off):
                got = True
                if ins.mnemonic in ("ret", "retf", "retn", "lret"):
                    sites.add(ins.address)
                elif ins.mnemonic == "jmp" and ins.op_str.startswith("0x") and depth < 3:
                    tgt = int(ins.op_str, 16)
                    if not (lin <= tgt < end):                       # a tail call into another routine
                        for n2 in self.tr.procs:
                            if self.tr.func_linear(n2) == tgt:
                                sites |= self.ret_sites(n2, depth + 1)
                off = ins.address + ins.size - lin
            if not got or off < len(code) and not got:
                off += 1
        return sites

    def guard(self, fn):
        def wrapped(*args):
            try:
                fn(*args)
            except BaseException as e:  # noqa: BLE001 -- carried out of the emulator callback
                self.exc = e
                self.stop = "exception"
                self.uc.emu_stop()
        return wrapped

    def first_ret_size(self, name):
        """2 / 4: size of the return address popped by the first ret / retf of the routine (None if not found)"""
        if not hasattr(self, "_rs"):
            self._rs = {}
        if name not in self._rs:
            lin = self.tr.func_linear(name)
            res = None
            if lin is not None:
                md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
                for ins in md.disasm(bytes(self.tape.mem0[lin:lin + 0x600]), lin):
                    if ins.mnemonic in ("ret", "retn"):
                        res = 2
                        break
                    if ins.mnemonic in ("retf", "lret"):
                        res = 4
                        break
            self._rs[name] = res
        return self._rs[name]

    def setup_hooks(self):
        uc = self.uc
        self.exc = None
        self.host_at = {}                                 # linear address -> canonical host name
        names = set()
        for path in self.a.gen:
            names |= set(canon(n) for n in re.findall(r"RHOST\(c, (\w+),", open(path).read()))
        for n in sorted(names):
            lin = self.tr.func_linear(n)
            if lin is None:
                continue
            lin = self.code(lin)
            self.host_at[lin] = n
            uc.hook_add(UC_HOOK_CODE, self.guard(self.on_host), None, lin, lin)
        self.fn_at = {}                                   # linear address -> translated routine (entry-point checks)
        if self.a.entry_checks:
            fn_names = set()
            for path in self.a.gen:
                fn_names |= set(re.findall(r"^static void fn_(\w+)\(Cpu \*c\);", open(path).read(), re.M))
            for n in sorted(fn_names):
                lin = self.tr.func_linear(n)
                lin = self.code(lin) if lin is not None else None
                if lin is not None and lin not in self.host_at:
                    self.fn_at[lin] = n
                    uc.hook_add(UC_HOOK_CODE, self.guard(self.on_func), None, lin, lin)
        self.label_at = {}                                # linear address -> label name (--labels)
        self.label_names = set()
        for fname in self.a.labels or []:
            if fname not in self.tr.procs:
                raise SystemExit("--labels: no such routine " + fname)
            st, en, _ = self.tr.procs[fname]
            for l in self.tr.lines[st:en]:
                m = re.match(r"^(loc_([0-9A-F]{5})):", l)
                if m:
                    lin = self.code(int(m.group(2), 16))
                    self.label_at[lin] = m.group(1)
                    self.label_names.add(m.group(1))
                    uc.hook_add(UC_HOOK_CODE, self.guard(self.on_label), None, lin, lin)
        self.ret_at = {}
        for n in sorted(names):
            if self.klass(n)[0] != "R":
                continue
            for site in self.ret_sites(n):
                self.ret_at.setdefault(self.code(site), set()).add(n)
        for site in self.ret_at:
            uc.hook_add(UC_HOOK_CODE, self.guard(self.on_ret), None, site, site)
        uc.hook_add(UC_HOOK_CODE, self.on_sentinel, None, SENT_SEG * 16, SENT_SEG * 16)
        if self.a.trace_last:
            self.ring = []
            uc.hook_add(UC_HOOK_CODE, self.on_trace)
        uc.hook_add(UC_HOOK_INTR, self.guard(self.on_intr))
        uc.hook_add(UC_HOOK_INSN, lambda uc, port, size, user: 0, None, 1, 0, UC_X86_INS_IN)
        uc.hook_add(UC_HOOK_INSN, lambda uc, port, size, value, user: None, None, 1, 0, UC_X86_INS_OUT)

    # register helpers
    def r(self, reg): return self.uc.reg_read(reg)
    def w(self, reg, v): self.uc.reg_write(reg, v & 0xFFFF)
    def stack_words(self, n, skip=0):
        base = self.r(UC_X86_REG_SS) * 16 + self.r(UC_X86_REG_SP) + skip
        return struct.unpack("<%dH" % n, bytes(self.uc.mem_read(base, 2 * n)))

    def redirect(self, cs, ip):
        self.w(UC_X86_REG_CS, cs)
        self.w(UC_X86_REG_IP, ip)
        self.stop = "redirect"
        self.uc.emu_stop()

    # ---------------------------------------------------------------- the interrupt hook: overlay thunks
    def on_intr(self, uc, intno, user):
        if intno == 0x3F:
            cs, ip = self.r(UC_X86_REG_CS), self.r(UC_X86_REG_IP)
            off = struct.unpack("<H", bytes(uc.mem_read(cs * 16 + ip, 2)))[0]
            seg = OVERLAY_SEGMENTS[STUB_SEGMENTS.index(cs)]
            self.redirect(OVL1_LOW_SEG if seg == OVL1_SEG else seg, off)
            return
        self.fail("unexpected INT %02Xh at %04X:%04X (ax=%04X)" % (intno, self.r(UC_X86_REG_CS), self.r(UC_X86_REG_IP), self.r(UC_X86_REG_AX)))

    # ---------------------------------------------------------------- comparison
    def where(self):
        return "%04X:%04X" % (self.r(UC_X86_REG_CS), self.r(UC_X86_REG_IP))

    def func_at(self, linear):
        """name of the routine containing a linear code address"""
        linear = self.uncode(linear)
        if not hasattr(self, "_starts"):
            self._starts = sorted((self.tr.func_linear(n), n) for n in self.tr.procs if self.tr.func_linear(n) is not None)
        best = None
        for a, n in self._starts:
            if a <= linear:
                best = (a, n)
            else:
                break
        return "%s+%X" % (best[1], linear - best[0]) if best else "?"

    def callers(self, depth=6):
        """the original's call chain, from the return addresses reachable through the bp chain"""
        out = []
        try:
            bp = self.r(UC_X86_REG_BP)
            sp = self.r(UC_X86_REG_SP)
            ss = self.r(UC_X86_REG_SS)
            # the immediate caller of the routine being entered: first word(s) on the stack
            ip, cs = struct.unpack("<HH", bytes(self.uc.mem_read(ss * 16 + sp, 4)))
            out.append(self.func_at(cs * 16 + ip) + (" (far)" if cs else ""))
            for _ in range(depth):
                if bp == 0 or bp >= 0xFFF0:
                    break
                nbp, ip, cs = struct.unpack("<HHH", bytes(self.uc.mem_read(ss * 16 + bp, 6)))
                out.append(self.func_at(cs * 16 + ip))
                bp = nbp
        except Exception:  # noqa: BLE001
            pass
        return out

    def sym(self, off):
        """name of the data symbol covering DGROUP offset `off` (from the IDA names), for readable reports"""
        if not hasattr(self, "_syms"):
            self._syms = sorted((v - DG_LIN, k) for k, v in self.tr.names.items() if DG_LIN <= v < DG_LIN + 0x10000)
        best = None
        for o, n in self._syms:
            if o <= off:
                best = (o, n)
            else:
                break
        return "%s%+d" % (best[1], off - best[0]) if best else "?"

    def compare_memory(self, what):
        limit = self.limit
        u = bytes(self.uc.mem_read(DG_LIN, limit))
        m = bytes(self.model[DG_LIN:DG_LIN + limit])
        if u == m:
            return
        diffs = []
        for i in range(0, limit, 8):
            if u[i:i + 8] != m[i:i + 8]:
                for j in range(i, min(i + 8, limit)):
                    if u[j] != m[j] and not any(lo <= j <= hi for lo, hi in self.ignore):
                        diffs.append(j)
        if not diffs:
            return
        lines = ["data segment differs at %s (%d bytes); first differences (offset: original / recompiled):" % (what, len(diffs))]
        for j in diffs[:24]:
            lines.append("    %04X %-28s %02X / %02X" % (j, self.sym(j), u[j], m[j]))
        self.fail("\n".join(lines))

    def compare_heap(self, what):
        """everything above the data segment (far heap: loaded maps, sprites, buffers) except the video memory"""
        lo, hi = DG_LIN + 0x10000, self.tape.memsize
        skip = (0xA0000, 0xB0000)
        ov = (OVL1_LOW_SEG * 16, OVL1_LOW_SEG * 16 + self.ovl1_size)         # the emulator's copy of overlay 1
        for a, b in ((lo, skip[0]), (skip[1], ov[0]), (ov[1], hi)):
            u = bytes(self.uc.mem_read(a, b - a))
            m = bytes(self.model[a:b])
            if u == m:
                continue
            diffs = [a + i for i in range(0, b - a, 16) if u[i:i + 16] != m[i:i + 16]]
            lines = ["far heap differs at %s (%d 16-byte blocks); first blocks (address: original / recompiled):" % (what, len(diffs))]
            for d in diffs[:8]:
                i = d - a
                lines.append("    %05X: %s / %s" % (d, u[i:i + 16].hex(), m[i:i + 16].hex()))
            self.fail("\n".join(lines))

    def fail(self, msg):
        out = ["", "DIVERGENCE after %d tape events, original code at %s" % (self.n_events, self.where()), msg, "",
               "original call chain: " + " <- ".join(self.callers()), "in routine: " + self.func_at(self.r(UC_X86_REG_CS) * 16 + self.r(UC_X86_REG_IP))]
        if getattr(self, "ring", None):
            out.append("last instructions: " + " ".join(self.func_at(a) for a in self.ring[-self.a.trace_last:]))
        out += ["", "recent calls:"]
        out.extend("    " + x for x in self.recent[-12:])
        raise Divergence("\n".join(out))

    def note(self, text):
        self.recent.append(text)
        if len(self.recent) > 64:
            del self.recent[:32]

    # ---------------------------------------------------------------- host call entry in the original code
    def on_host(self, uc, address, size, user):
        name = self.host_at[address]
        cls, retw = self.klass(name)
        ev = self.peek()
        if ev is None:
            self.top_done = True
            self.stop = "tape end"
            uc.emu_stop()
            return
        evname = canon(ev["name"]) if ev["t"] == "H" else None
        if self.frames and (ev["t"] == "h" or evname != name) and cls == "R":
            return                                    # a helper the replaced routine calls internally: just run it
        if ev["t"] in "TU":
            if ev["t"] == "U":
                self.fail("the recompiled game returned from its top-level routine but the original called %s" % name)
            self.fail("unexpected top-level event %s" % ev["name"])
        if ev["t"] != "H" or evname != name:
            exp = ("%s(%s)" % (evname, ", ".join("%04X" % w for w in ev["stack"][:ev["nargs"]]))) if ev["t"] == "H" else "return from a host routine"
            self.fail("the original code calls %s, the recompiled game expected %s" % (name, exp))
        self.take()
        self.apply_delta(ev)
        sp = self.r(UC_X86_REG_SP)
        ret_size = ev["ret"] if ev["regs"]["sp"] else (4 if self.tr.procs.get(name, [0, 0, "far"])[2] == "far" else 2)
        kind = self.first_ret_size(name)
        if kind and kind != ret_size:
            self.fail("%s: the translated call assumes a %d byte return address, the original routine's first return instruction pops %d" % (name, ret_size, kind))
        if not self.frames:       # (inside a replaced routine the intermediate state may legitimately differ: it is compared at its end)
            self.compare_memory("the call of %s" % name)
        nargs = STACK_ARGS.get(name, ev["nargs"])
        args_u = self.stack_words(max(nargs, 0), ret_size)
        args_c = ev["stack"][:nargs]
        for reg in REG_ARGS.get(name, ()):
            ureg = {"ax": UC_X86_REG_AX, "bx": UC_X86_REG_BX, "cx": UC_X86_REG_CX, "dx": UC_X86_REG_DX}[reg]
            if self.r(ureg) != ev["regs"][reg]:
                self.fail("%s: register %s differs at the call: original %04X, recompiled %04X" % (name, reg, self.r(ureg), ev["regs"][reg]))
        if ev["regs"]["sp"] and not self.frames and ev["regs"]["sp"] != (sp + ret_size) & 0xFFFF:
            self.fail("%s: stack pointer differs: original %04X (+%d return address), recompiled %04X -- the translated callers have a different stack depth\n"
                      "    frame pointer bp: original %04X, recompiled %04X" % (name, sp, ret_size, ev["regs"]["sp"], self.r(UC_X86_REG_BP), ev["regs"]["bp"]))
        if tuple(args_u) != tuple(args_c):
            self.fail("%s: arguments differ: original (%s), recompiled (%s)" % (name, " ".join("%04X" % w for w in args_u), " ".join("%04X" % w for w in args_c)))
        for reg, ureg in (("si", UC_X86_REG_SI), ("di", UC_X86_REG_DI), ("bp", UC_X86_REG_BP)):
            if ev["regs"]["sp"] and not self.frames and self.r(ureg) != ev["regs"][reg]:     # (a call made by readable C has no registers to compare)
                self.fail("%s: register %s differs at the call: original %04X, recompiled %04X" % (name, reg, self.r(ureg), ev["regs"][reg]))
        self.note("%s(%s)" % (name, ", ".join("%04X" % w for w in args_u)))
        self.host_since_label = True
        if self.a.heap_every and self.n_events // self.a.heap_every != self.last_heap:
            self.last_heap = self.n_events // self.a.heap_every
            self.compare_heap("the call of %s" % name)
        st = self.stats.setdefault(name, [cls, 0])
        st[1] += 1
        if cls == "E":
            self.replay_effect(name, sp, ret_size, ev["regs"]["sp"])
        else:
            self.frames.append({"name": name, "sp": sp, "ret": ret_size, "retw": retw, "args": args_u})

    def on_func(self, uc, address, size, user):
        """the original enters a routine the recompiled game also translated: same routine, same stack pointer"""
        name = self.fn_at[address]
        ev = self.peek()
        if ev is None:
            return
        if ev["t"] != "F" or ev["name"] != name:
            if self.frames:
                return                                   # inside a replaced routine: its helpers need not be on the tape
            exp = "enters %s" % ev["name"] if ev["t"] == "F" else ("calls host routine %s" % ev["name"] if ev["t"] == "H" else "ends")
            self.fail("the original enters %s, the recompiled game %s" % (name, exp))
        self.take()
        sp, bp = self.r(UC_X86_REG_SP), self.r(UC_X86_REG_BP)
        self.note("> " + name)
        self.fn_counts[name] = self.fn_counts.get(name, 0) + 1
        if self.frames:           # called back from a readable rewrite: its stack frame is not the original's, so the depth differs by design
            return
        if ev["regs"]["sp"] != sp:
            self.fail("%s entered with stack pointer %04X in the original, %04X in the recompiled game (a callee earlier left a different amount on the stack)"
                      % (name, sp, ev["regs"]["sp"]))
        if ev["regs"]["bp"] != bp:
            self.fail("%s entered with bp %04X in the original, %04X in the recompiled game" % (name, bp, ev["regs"]["bp"]))

    def on_label(self, uc, address, size, user):
        """--labels: the original reaches a label of the routine under test; the recompiled game must have reached the same one with the same registers"""
        name = self.label_at[address]
        ev = self.peek()
        if ev is None:
            return
        if ev["t"] != "L" or ev["name"] != name:
            got = ev["name"] if ev["t"] in "LFH" else ev["t"]
            self.fail("the original reaches label %s, the recompiled game reaches %s next -- the paths diverge here" % (name, got))
        self.take()
        self.apply_delta(ev)
        regs = ev["regs"]
        self.note("label " + name)
        if not self.frames:
            self.compare_memory("label %s (the block before it stored something different)" % name)
        diffs = []
        after_host = self.host_since_label          # (a call has happened in this top-level routine: bx, cx, dx may be leftovers of the callee)
        for reg, ureg in (("ax", UC_X86_REG_AX), ("bx", UC_X86_REG_BX), ("cx", UC_X86_REG_CX), ("dx", UC_X86_REG_DX), ("si", UC_X86_REG_SI),
                          ("di", UC_X86_REG_DI), ("bp", UC_X86_REG_BP), ("sp", UC_X86_REG_SP)):
            if after_host and reg in ("bx", "cx", "dx"):
                continue                      # a host routine (the original's or its replacement) leaves these as it likes
            if self.r(ureg) != regs[reg]:
                diffs.append("%s: original %04X, recompiled %04X" % (reg, self.r(ureg), regs[reg]))
        if diffs and not self.frames:
            self.fail("registers differ at label %s (the first label where they do; a register that is dead here may differ harmlessly):\n    %s"
                      % (name, "\n    ".join(diffs)))

    def replay_effect(self, name, sp, ret_size, c_sp_before):
        """skip the original routine: apply what the recorded host changed, return its registers"""
        touched = []
        depth = 0
        while True:
            e = self.take()
            if e["t"] == "H":
                depth += 1
            self.apply_delta(e)
            touched.extend((off, len(d)) for off, d in e["delta"])
            if e["t"] == "h":
                if depth == 0:
                    break
                depth -= 1
        for off, ln in touched:
            self.uc.mem_write(off, bytes(self.model[off:off + ln]))
            if off < DG_LIN + 0x10000 and off + ln > DG_LIN:       # remember which data segment bytes the host wrote (tools/mm3_hostwrites.py)
                w = self.writes.setdefault(name, set())
                for o in range(max(off, DG_LIN), min(off + ln, DG_LIN + 0x10000)):
                    w.add(o - DG_LIN)
        self.w(UC_X86_REG_AX, e["ax"])
        self.w(UC_X86_REG_DX, e["dx"])
        self.w(UC_X86_REG_ES, e["es"])
        # (a host that pops its own arguments moved the recompiled game's stack pointer; apply the same change in the original's frame)
        self.do_return(ret_size, (sp + ret_size + ((e["sp"] - c_sp_before) & 0xFFFF)) & 0xFFFF if e["sp"] and c_sp_before else None)

    def do_return(self, ret_size, new_sp=None):
        """return from the skipped routine; `new_sp` (the host's stack pointer afterwards) accounts for routines that pop their own arguments"""
        ss = self.r(UC_X86_REG_SS)
        sp = self.r(UC_X86_REG_SP)
        words = struct.unpack("<%dH" % (ret_size // 2), bytes(self.uc.mem_read(ss * 16 + sp, ret_size)))
        self.w(UC_X86_REG_SP, new_sp if new_sp is not None else sp + ret_size)
        self.redirect(words[1] if ret_size == 4 else self.r(UC_X86_REG_CS), words[0])

    # ---------------------------------------------------------------- return from an R routine
    def on_ret(self, uc, address, size, user):
        if not self.frames:
            return
        fr = self.frames[-1]
        if fr["name"] not in self.ret_at.get(address, ()) or self.r(UC_X86_REG_SP) != fr["sp"]:
            return
        if self.peek() is None:               # the recorded run ended inside this routine (the recompiled game stopped or was aborted)
            self.stop = "tape end"
            uc.emu_stop()
            return
        ev = self.take() if self.peek()["t"] == "h" else None
        if ev is None:
            self.fail("%s returned in the original but the recompiled routine made further calls (next on tape: %s)"
                      % (fr["name"], self.peek() and self.peek().get("name")))
        self.apply_delta(ev)
        self.compare_memory("the return of %s" % fr["name"])
        if fr["retw"] >= 1 and self.r(UC_X86_REG_AX) != ev["ax"]:
            if (self.r(UC_X86_REG_AX) ^ ev["ax"]) & 0xFF:
                self.fail("%s(%s) returns ax=%04X in the original, %04X in the recompiled game"
                          % (fr["name"], " ".join("%04X" % w for w in fr["args"]), self.r(UC_X86_REG_AX), ev["ax"]))
            # only the high byte differs: the original returns a char in al and leaves ah as it was (callers extend it themselves)
            self.stats[fr["name"]].append("ah") if "ah" not in self.stats[fr["name"]] else None
        if fr["retw"] >= 2 and self.r(UC_X86_REG_DX) != ev["dx"]:
            self.fail("%s(%s) returns dx=%04X in the original, %04X in the recompiled game"
                      % (fr["name"], " ".join("%04X" % w for w in fr["args"]), self.r(UC_X86_REG_DX), ev["dx"]))
        self.frames.pop()

    def on_trace(self, uc, address, size, user):
        self.ring.append(address)
        if len(self.ring) > self.a.trace_last:
            del self.ring[:len(self.ring) - self.a.trace_last]

    def on_sentinel(self, uc, address, size, user):
        self.stop = "top"
        uc.emu_stop()

    # ---------------------------------------------------------------- top-level routines
    def run(self):
        uc = self.uc
        try:
            while True:
                try:
                    ev = self.peek()
                    if ev is None:
                        break
                    if ev["t"] != "T":
                        self.fail("tape event %r outside a top-level routine" % ev["t"])
                    self.take()
                    self.apply_delta(ev, to_uc=True)     # the bring-up code between top-level calls changed memory
                    self.run_top(ev)
                except Aborted:
                    continue
        except StopIteration:
            pass
        except UcError as e:
            self.fail("emulator error %s" % e)

    def run_top(self, ev):
        uc = self.uc
        name = ev["name"]
        lin = self.tr.func_linear(name)
        seg = segment_of(lin)
        regs = ev["regs"]
        if seg == OVL1_SEG:
            seg = OVL1_LOW_SEG
        for reg, ureg in (("ax", UC_X86_REG_AX), ("bx", UC_X86_REG_BX), ("cx", UC_X86_REG_CX), ("dx", UC_X86_REG_DX),
                          ("si", UC_X86_REG_SI), ("di", UC_X86_REG_DI), ("bp", UC_X86_REG_BP), ("es", UC_X86_REG_ES),
                          ("ds", UC_X86_REG_DS)):
            self.w(ureg, regs[reg])
        self.w(UC_X86_REG_SS, DSEG)
        sp = regs["sp"]
        self.w(UC_X86_REG_SP, sp)
        uc.mem_write(DG_LIN + sp, struct.pack("<HH", 0, SENT_SEG))        # far return address of the top-level call
        self.frames = []
        self.host_since_label = False
        cs, ip = seg, self.code(lin) - seg * 16
        self.note("TOP %s" % name)
        while True:
            self.stop = None
            self.w(UC_X86_REG_CS, cs)
            self.w(UC_X86_REG_IP, ip)
            uc.emu_start(cs * 16 + ip, 0, timeout=self.a.timeout * 1000000)
            if self.stop == "exception":
                if isinstance(self.exc, Aborted):
                    self.note("(recompiled routine aborted)")
                    return
                raise self.exc
            if self.stop == "redirect":
                cs, ip = self.r(UC_X86_REG_CS), self.r(UC_X86_REG_IP)
                continue
            break
        if self.stop == "tape end":
            raise StopIteration
        if self.stop != "top":
            self.fail("the original code did not return (stopped at %s; probably waiting in a loop the recompiled game does not have)" % self.where())
        e = self.take() if self.peek() and self.peek()["t"] == "U" else None
        if e is None:
            self.fail("the original routine %s returned but the recompiled one is still calling (%s)" % (name, self.peek() and self.peek().get("name")))
        self.apply_delta(e)
        self.compare_memory("the end of %s" % name)
        self.compare_heap("the end of %s" % name)

    def report(self):
        print("%d tape events matched" % self.n_events)
        print("%-28s %-3s %8s" % ("host routine", "cls", "calls"))
        for n, st in sorted(self.stats.items(), key=lambda kv: (kv[1][0], -kv[1][1])):
            print("%-28s %-3s %8d%s" % (n, st[0], st[1], "   (original leaves ah unspecified: char result)" if "ah" in st[2:] else ""))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("tape")
    ap.add_argument("--asm", default=os.path.join(HERE, "..", "mm3.asm"))
    ap.add_argument("--idc", default=os.path.join(HERE, "..", "mm3.idc"))
    ap.add_argument("--gen", action="append", default=[os.path.join(HERE, "..", "src", "gen", "game_gen.c")],
                    help="generated sources whose host calls are intercepted")
    ap.add_argument("--dg-limit", type=lambda s: int(s, 0), default=0xF200, help="bytes of the data segment compared (the rest is the stack)")
    ap.add_argument("--ignore", action="append", help="data segment offset range LO-HI (hex) not compared")
    ap.add_argument("--run", action="append", help="treat this host routine as R (run and compare)")
    ap.add_argument("--replay", action="append", help="treat this host routine as E (replay the recorded effect)")
    ap.add_argument("--stats", help="write the per-routine call counts of this run to this JSON file (see tools/mm3_coverage.py)")
    ap.add_argument("--trace-last", type=int, default=0, help="on a divergence, show the routine+offset of the last N executed instructions of the original (slow)")
    ap.add_argument("--image", default=os.path.join(HERE, "..", "data", "IMAGE.BIN"), help="the derived program image (make derive): pristine overlay code")
    ap.add_argument("--labels", action="append", help="routine to compare label by label (needs a tape from mm3game_labels): the first label where the registers differ")
    ap.add_argument("--timeout", type=int, default=60)
    ap.add_argument("--heap-every", type=int, default=20000, help="compare the far heap every N tape events (0: only at the end of top-level routines)")
    ap.add_argument("--no-entry-checks", dest="entry_checks", action="store_false", help="do not compare the entry of translated routines")
    a = ap.parse_args()
    ls = Lockstep(a)
    rc = 0
    try:
        ls.run()
        print("NO DIVERGENCE")
    except Divergence as d:
        print(d)
        rc = 1
    ls.report()
    if a.stats:
        import json
        json.dump({"events": ls.n_events, "diverged": bool(rc), "calls": {n: {"class": st[0], "count": st[1]} for n, st in ls.stats.items()}, "functions": ls.fn_counts,
                   "writes": {n: sorted(v) for n, v in ls.writes.items()}},
                  open(a.stats, "w"), indent=1)
    return rc


if __name__ == "__main__":
    sys.exit(main())
