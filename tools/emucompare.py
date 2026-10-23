#!/usr/bin/env python3
"""Runs the same functions of two original executables (e.g. Deponia and Deponia4) on the same arguments in an x86-64 emulator
(Unicorn) and compares what they return. A condition or a constant that was changed shows up as another result, however the
compilers wrote it (-O0 against -O2 does not matter).

Only functions whose arguments are numbers can be run: int, unsigned, long, bool (0 or 1), char, enum (names like eFoo, FooEnum, TypeFoo),
float and double. Free functions and the static methods named in the sources (`static` in the headers of --headers) are tried;
a function that faults, takes longer than 1.5 s or stops in an assert/abort is skipped. The libraries are not loaded: their PLT stubs are noted and answered.

Usage: tools/emucompare.py <first exe> <second exe> [--headers DIR] [--filter TEXT] [--list] [--methods]
  --methods       the non-static methods of the T* classes instead: `this` is a block of 4 KB of random small numbers (the same in both
                  runs), the results and what the method wrote into the block are compared. A class whose members moved between the
                  builds differs everywhere; for the others a difference is a difference of the code. Methods that call through a
                  pointer in the block (a virtual call, another object) fault and are skipped.
  --trials N      how many argument sets / random blocks for each method (10): a test against a number is only seen when a block has that number
  --headers DIR   the sources whose `static` methods are tried too (default src/deponia4)
  --filter TEXT   only the functions whose demangled name contains TEXT
Prints per function: same results on all inputs, or the first inputs that differ. Needs nm and c++filt on the PATH, and unicorn."""
import math
import os
import random
import re
import struct
import subprocess
import sys

from unicorn import (UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_FETCH_UNMAPPED, UC_HOOK_MEM_READ, UC_HOOK_MEM_READ_UNMAPPED, UC_HOOK_MEM_WRITE_UNMAPPED, UC_MODE_64,
                     Uc, UcError)
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_R8, UC_X86_REG_R9, UC_X86_REG_RAX, UC_X86_REG_RCX, UC_X86_REG_RDI,
                               UC_X86_REG_RDX, UC_X86_REG_RIP, UC_X86_REG_RSI, UC_X86_REG_RSP, UC_X86_REG_XMM0, UC_X86_REG_XMM1,
                               UC_X86_REG_XMM2, UC_X86_REG_XMM3, UC_X86_REG_XMM4, UC_X86_REG_XMM5, UC_X86_REG_XMM6, UC_X86_REG_XMM7,
                               UC_X86_REG_FS_BASE)

INT_REGS = [UC_X86_REG_RDI, UC_X86_REG_RSI, UC_X86_REG_RDX, UC_X86_REG_RCX, UC_X86_REG_R8, UC_X86_REG_R9]
XMM_REGS = [UC_X86_REG_XMM0, UC_X86_REG_XMM1, UC_X86_REG_XMM2, UC_X86_REG_XMM3, UC_X86_REG_XMM4, UC_X86_REG_XMM5, UC_X86_REG_XMM6, UC_X86_REG_XMM7]
STACK = 0x7FFF00000000
STACK_SIZE = 0x100000
SENTINEL = 0x7FFE00000000
STUB = SENTINEL + 0x1000     # where a call through a pointer that is not there ends: a `ret`
TLS = 0x7FFD00000000
HEAP = 0x7FFC00000000
HEAP_SIZE = 0x20000
OBJECT_BYTES = 0x1000
ENUM = re.compile(r"^(e[A-Z]\w*|\w*Enum|Type\w+|TypeOrder)$")
INT_TYPES = {"int", "unsigned int", "unsigned", "long", "unsigned long", "short", "unsigned short", "char", "unsigned char", "signed char",
             "bool", "long long", "unsigned long long"}


MATH = {"sqrt": math.sqrt, "sin": math.sin, "cos": math.cos, "tan": math.tan, "atan": math.atan, "atan2": math.atan2, "asin": math.asin,
        "acos": math.acos, "pow": math.pow, "floor": math.floor, "ceil": math.ceil, "fabs": math.fabs, "exp": math.exp, "log": math.log,
        "fmod": math.fmod, "round": lambda x: float(math.floor(x + 0.5)) if x >= 0 else -float(math.floor(-x + 0.5)), "trunc": math.trunc,
        "log10": math.log10, "sinh": math.sinh, "cosh": math.cosh, "tanh": math.tanh, "hypot": math.hypot}
MATH_ARGS = {"atan2": 2, "pow": 2, "fmod": 2, "hypot": 2}
ARENA = 0x7FFB00000000
ARENA_SIZE = 0x800000
BLOCK = 0x1000
BLOCK_STRIDE = 0x2000
MAX_INSNS = 300000
IMAGE_TOKEN_END = 0x1800000  # (the addresses of code and data of both builds; Deponia 4 has 16 MB of bss above it, which small numbers must not be taken for)
RUN_MICROSECONDS = 1500000   # (a call that takes longer than this is given up)
# calls whose presence or order says nothing about the logic (the optimizer inlines or merges them)
NOISE = {"_Znwm", "_Znam", "_ZdlPv", "_ZdaPv", "_ZdlPvm", "malloc", "calloc", "realloc", "free", "memcpy", "memmove", "memset", "memcmp",
         "strlen", "strcmp", "strncmp", "strchr", "strcpy", "strncpy", "__cxa_guard_acquire", "__cxa_guard_release", "__cxa_guard_abort",
         "__cxa_atexit", "__cxa_begin_catch", "__cxa_end_catch", "__cxa_rethrow", "__cxa_allocate_exception", "__cxa_throw",
         "_Unwind_Resume", "__gxx_personality_v0", "__stack_chk_fail", "abort", "__assert_fail", "__cxa_pure_virtual",
         "_ZSt9terminatev", "_ZNSs4_Rep20_S_empty_rep_storageE", "clock_gettime", "gettimeofday", "time", "pthread_mutex_lock",
         "pthread_mutex_unlock", "pthread_mutex_init", "pthread_mutexattr_init", "pthread_mutexattr_settype", "pthread_mutex_destroy"}
NOISE |= set(MATH) | {n + "f" for n in MATH} | {"sincos", "sincosf"}  # (the optimizer makes a call of the library of sqrt, floor ... an instruction)
NEEDS_ARGS = {"sincosf", "sincos", "clock_gettime", "gettimeofday", "time", "av_malloc", "av_mallocz", "av_realloc", "av_calloc", "av_malloc_array", "av_mallocz_array", "fwrite", "rand", "_Znwm", "_Znam", "malloc", "calloc", "_Znwj", "_Znaj", "memcpy", "memmove", "memset", "strlen", "memcmp", "strncmp", "strcmp"} | set(MATH) | {n + "f" for n in MATH}
STRING_PREFIXES = ("_ZNSaI", "_ZNSs", "_ZNKSs", "_ZNSbIw", "_ZNKSbIw", "_ZSt", "_ZNSt", "_ZNKSt", "_ZStl", "_ZStp", "_ZSteq", "_ZStne")
STOPPERS = {"__stack_chk_fail", "abort", "__assert_fail", "__cxa_throw", "_Unwind_Resume", "__cxa_pure_virtual", "_ZSt9terminatev"}
NEEDS_ARGS |= STOPPERS


class Image:
    """One executable in an emulator: its PT_LOAD segments mapped at their addresses, a stack, a thread block, blocks of memory for the
    objects given to the function, an arena for what it allocates and pages of zeros for whatever else it reads. The calls into the
    libraries (the PLT stubs) are not run: they are noted (name and number arguments) and answered (new, malloc, memcpy, memset,
    strlen and the compare functions really, anything else 0). A call through a pointer that is not there returns at once."""

    def __init__(self, path):
        data = open(path, "rb").read()
        phoff, = struct.unpack_from("<Q", data, 0x20)
        phentsize, phnum = struct.unpack_from("<HH", data, 0x36)
        segs = []
        for i in range(phnum):
            p_type, flags, off, vaddr, paddr, filesz, memsz, align = struct.unpack_from("<IIQQQQQQ", data, phoff + i * phentsize)
            if p_type == 1:
                segs.append((vaddr, off, filesz, memsz))
        low = min(s[0] for s in segs) & ~0xFFF
        high = (max(s[0] + s[3] for s in segs) + 0xFFF) & ~0xFFF
        self.uc = Uc(UC_ARCH_X86, UC_MODE_64)
        self.uc.mem_map(low, high - low)
        for vaddr, off, filesz, memsz in segs:
            self.uc.mem_write(vaddr, data[off:off + filesz])
        self.uc.mem_map(STACK, STACK_SIZE)
        self.uc.mem_map(SENTINEL, 0x2000)
        self.uc.mem_write(STUB, bytes([0xC3]) * 0x1000)
        self.uc.mem_map(TLS, 0x2000)
        self.uc.mem_map(HEAP, HEAP_SIZE)
        self.uc.mem_map(ARENA, ARENA_SIZE)
        self.uc.mem_write(TLS + 0x28, b"\x11\x22\x33\x44\x55\x66\x77\x88")  # (the stack protector's canary)
        self.uc.mem_write(TLS, struct.pack("<Q", TLS))
        self.low, self.high = low, high
        self.arena_top = ARENA
        self.pages = []      # the pages mapped on demand
        self.trace = []
        self.stopped = None
        self.plt_names = {}
        self._find_plt(data)
        self.low_pages = []
        self.uc.hook_add(UC_HOOK_MEM_READ_UNMAPPED | UC_HOOK_MEM_WRITE_UNMAPPED, self._unmapped)
        self.uc.hook_add(UC_HOOK_MEM_READ, self._small_read, begin=0, end=0xFFFF)
        self.uc.hook_add(UC_HOOK_CODE, self._virtual, begin=STUB, end=STUB + 0xFFF)
        self.uc.hook_add(UC_HOOK_MEM_FETCH_UNMAPPED, self._fetch_unmapped)
        for begin, end in self.plt_ranges:
            self.uc.hook_add(UC_HOOK_CODE, self._plt, begin=begin, end=end)

    def _find_plt(self, data):
        """The stubs of the PLT: their address -> the name of the function in the library (from .rela.plt)."""
        shoff, = struct.unpack_from("<Q", data, 0x28)
        shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 0x3A)
        sections = []
        for i in range(shnum):
            name, typ, flags, addr, off, size, link, info, align, entsize = struct.unpack_from("<IIQQQQIIQQ", data, shoff + i * shentsize)
            sections.append((name, typ, addr, off, size, link, entsize))
        strtab = sections[shstrndx]
        names = {}
        for s in sections:
            end = data.index(b"\0", strtab[3] + s[0])
            names[data[strtab[3] + s[0]:end].decode()] = s
        self.plt_ranges = []
        got_names = {}
        rela, dynsym, dynstr = names.get(".rela.plt"), names.get(".dynsym"), names.get(".dynstr")
        if rela and dynsym and dynstr:
            for k in range(rela[4] // 24):
                r_offset, r_info, r_addend = struct.unpack_from("<QQq", data, rela[3] + k * 24)
                sym = r_info >> 32
                st_name, = struct.unpack_from("<I", data, dynsym[3] + sym * 24)
                end = data.index(b"\0", dynstr[3] + st_name)
                got_names[r_offset] = data[dynstr[3] + st_name:end].decode()
        for section in (".plt", ".plt.got", ".plt.sec"):
            s = names.get(section)
            if not s or not s[2]:
                continue
            self.plt_ranges.append((s[2], s[2] + s[4] - 1))
            for a in range(s[2], s[2] + s[4] - 5, 16 if section != ".plt.got" else 8):
                body = bytes(self.uc.mem_read(a, 8))
                if body[0] == 0xFF and body[1] == 0x25:
                    disp, = struct.unpack_from("<i", body, 2)
                    slot = a + 6 + disp
                    if slot in got_names:
                        self.plt_names[a] = got_names[slot]
                        self.uc.mem_write(a, b"\xC3")  # (the stub returns at once; the hook answers)

    def _unmapped(self, uc, access, address, size, value, user):
        page = address & ~0xFFF
        if page < 0 or page >= 0x7FFFFFFFF000:
            return False
        try:
            uc.mem_map(page, 0x1000)
        except UcError:
            return False
        if page < 0x10000:
            # (low addresses are slots of a table of virtual functions that is not there: each holds the address of a `ret`)
            uc.mem_write(page, struct.pack("<Q", STUB) * 0x200)
            self.low_pages.append(page)
            self.last_small = address
        else:
            self.pages.append(page)
        return True

    def _small_read(self, uc, access, address, size, value, user):
        self.last_small = address

    def _virtual(self, uc, address, size, user):
        self.trace.append(("virtual", ()))

    def _fetch_unmapped(self, uc, access, address, size, value, user):
        """A jump to a place that is not there (a virtual call through a table of zeros): a `ret`."""
        page = address & ~0xFFF
        if page < 0 or page >= 0x7FFFFFFFF000:
            return False
        try:
            uc.mem_map(page, 0x1000)
            uc.mem_write(page, b"\xC3" * 0x1000)
        except UcError:
            return False
        self.pages.append(page)
        self.trace.append(("virtual", ()))
        return True

    def _plt(self, uc, address, size, user):
        name = self.plt_names.get(address)
        if name is None:
            return
        self.trace.append((name, ()))
        if name not in NEEDS_ARGS:
            uc.reg_write(UC_X86_REG_RAX, 0)
            return
        args = tuple(uc.reg_read(r) for r in INT_REGS[:3])
        rax = 0
        if name in ("_Znwm", "_Znam", "malloc", "calloc", "_Znwj", "_Znaj"):
            n = args[0] if name != "calloc" else args[0] * args[1]
            n = (n + 15) & ~15
            if n > 0x100000 or self.arena_top + n > ARENA + ARENA_SIZE:
                self.stopped = "arena"
                uc.emu_stop()
                return
            rax = self.arena_top
            self.arena_top += n
        elif name in ("memcpy", "memmove") and args[2] <= 0x100000:
            uc.mem_write(args[0], bytes(uc.mem_read(args[1], args[2])))
            rax = args[0]
        elif name == "memset" and args[2] <= 0x100000:
            uc.mem_write(args[0], bytes([args[1] & 0xFF]) * args[2])
            rax = args[0]
        elif name == "strlen":
            data = bytes(uc.mem_read(args[0], 0x1000))
            rax = data.index(bytes(1)) if bytes(1) in data else 0x1000
        elif name in ("memcmp", "strncmp", "strcmp"):
            n = args[2] if name != "strcmp" else 0x10000
            a, b = bytes(uc.mem_read(args[0], min(n, 0x1000))), bytes(uc.mem_read(args[1], min(n, 0x1000)))
            if name != "memcmp":
                a, b = a.split(b"\0")[0], b.split(b"\0")[0]
            rax = (a > b) - (a < b) & 0xFFFFFFFF
        elif name in MATH or (name.endswith("f") and name[:-1] in MATH):
            single = name.endswith("f") and name[:-1] in MATH
            base = name[:-1] if single else name
            fmt, width = ("<f", 4) if single else ("<d", 8)
            xs = [struct.unpack(fmt, uc.reg_read(r).to_bytes(16, "little")[:width])[0] for r in XMM_REGS[:2]]
            try:
                value = MATH[base](*xs[:MATH_ARGS.get(base, 1)])
            except (ValueError, OverflowError, ZeroDivisionError):
                value = float("nan")
            uc.reg_write(UC_X86_REG_XMM0, int.from_bytes(struct.pack(fmt, value) + bytes(16 - width), "little"))
            return
        elif name in ("sincosf", "sincos"):
            single = name == "sincosf"
            fmt, width = ("<f", 4) if single else ("<d", 8)
            x, = struct.unpack(fmt, uc.reg_read(XMM_REGS[0]).to_bytes(16, "little")[:width])
            try:
                pair = (math.sin(x), math.cos(x))
            except ValueError:
                pair = (float("nan"), float("nan"))
            uc.mem_write(args[0], struct.pack(fmt, pair[0]))
            uc.mem_write(args[1], struct.pack(fmt, pair[1]))
            return
        elif name in ("clock_gettime", "gettimeofday"):
            uc.mem_write(args[1] if name == "clock_gettime" else args[0], struct.pack("<QQ", 1000, 500000))
        elif name == "time":
            rax = 1000
        elif name in ("av_malloc", "av_mallocz", "av_realloc", "av_calloc", "av_malloc_array", "av_mallocz_array"):
            n = (args[0] + 15) & ~15 if name not in ("av_calloc", "av_malloc_array", "av_mallocz_array") else (args[0] * args[1] + 15) & ~15
            if n > 0x100000 or self.arena_top + n > ARENA + ARENA_SIZE:
                self.stopped = "arena"
                uc.emu_stop()
                return
            rax = self.arena_top
            self.arena_top += n
        elif name == "fwrite":
            rax = args[2]
        elif name == "rand":
            self.rand_state = (getattr(self, "rand_state", 1) * 1103515245 + 12345) & 0x7FFFFFFF
            rax = self.rand_state
        elif name in STOPPERS:
            self.stopped = name
            uc.emu_stop()
            return
        uc.reg_write(UC_X86_REG_RAX, rax)
        uc.reg_write(UC_X86_REG_XMM0, 0)

    def call(self, address, ints, floats, blocks=()):
        """Calls the function: `ints` in the integer registers (a member of `ints` that is a number >= HEAP is the address of a block),
        `floats` as (value, is_double) in the xmm registers, `blocks` [(address, bytes)] written to memory before. Returns None when the
        function could not be run, else a dict: rax, xmm0, the blocks after the call (the pointers in them are tokens), the trace of calls."""
        uc = self.uc
        # (the pages that the last call mapped on demand are let go, so that they do not pile up)
        for page in self.pages + self.low_pages:
            try:
                uc.mem_unmap(page, 0x1000)
            except UcError:
                pass
        self.pages, self.low_pages = [], []
        if self.arena_top > ARENA:
            uc.mem_write(ARENA, bytes(self.arena_top - ARENA))
        self.arena_top = ARENA
        self.trace = []
        self.stopped = None
        self.rand_state = 1
        self.last_small = -1
        uc.reg_write(UC_X86_REG_FS_BASE, TLS)
        sp = STACK + STACK_SIZE - 0x1000
        uc.mem_write(STACK + STACK_SIZE - 0x3000, bytes(0x3000))
        uc.mem_write(sp, struct.pack("<Q", SENTINEL))
        uc.reg_write(UC_X86_REG_RSP, sp)
        uc.mem_write(HEAP, bytes(HEAP_SIZE))
        for a, content in blocks:
            uc.mem_write(a, content)
        for reg, v in zip(INT_REGS, ints):
            uc.reg_write(reg, v & 0xFFFFFFFFFFFFFFFF)
        for reg, (v, is_double) in zip(XMM_REGS, floats):
            raw = struct.pack("<d", v) if is_double else struct.pack("<f", v)
            uc.reg_write(reg, int.from_bytes(raw.ljust(16, b"\0"), "little"))
        uc.reg_write(UC_X86_REG_RAX, 0)
        try:
            uc.emu_start(address, SENTINEL, timeout=RUN_MICROSECONDS, count=MAX_INSNS)
        except UcError:
            return None
        if self.stopped or uc.reg_read(UC_X86_REG_RIP) != SENTINEL:
            return None
        after = []
        for a, content in blocks:
            raw = bytes(uc.mem_read(a, len(content)))
            after.append(self._tokens(raw))
        return {"rax": uc.reg_read(UC_X86_REG_RAX), "xmm0": uc.reg_read(UC_X86_REG_XMM0).to_bytes(16, "little")[:8],
                "blocks": after, "trace": [t[0] for t in self.trace if t[0] not in NOISE and t[0] != "virtual" and not t[0].startswith(STRING_PREFIXES)],  # (virtual: the stub answers the same, the number of such calls depends on what the optimizer devirtualized)
                "strings": any(t[0].startswith(STRING_PREFIXES) for t in self.trace)}

    def _tokens(self, raw):
        """The block as numbers, 8 bytes at a time, where an address of this image, the stack or the arena is a token (they differ by build)."""
        out = []
        for i in range(0, len(raw) - 7, 8):
            q, = struct.unpack_from("<Q", raw, i)
            if 0x400000 <= q < IMAGE_TOKEN_END or STACK <= q < STACK + STACK_SIZE or ARENA <= q < ARENA + ARENA_SIZE or SENTINEL <= q < SENTINEL + 0x1000:
                q = -1
            out.append(q)
        return out


def symbols(path):
    out = subprocess.run(["nm", "--defined-only", path], capture_output=True, text=True).stdout
    addr = {}
    for line in out.splitlines():
        p = line.split()
        if len(p) == 3 and p[1] in "TtWw":
            addr.setdefault(p[2], int(p[0], 16))
    return addr


def demangle(names):
    names = sorted(names)
    out = subprocess.run(["c++filt"], input="\n".join(names), capture_output=True, text=True).stdout.splitlines()
    return dict(zip(names, out))


def split_args(text):
    parts, depth, cur = [], 0, ""
    for ch in text:
        if ch in "<(":
            depth += 1
        elif ch in ">)":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        parts.append(cur.strip())
    return parts


def classify(demangled):
    """The kinds of the arguments ('i', 'f', 'd') or None when one is not a number."""
    m = re.match(r"^(?:[\w:<>~, *&]+\s)?([\w:~<>]+)\((.*)\)(?: const)?$", demangled)
    if not m:
        return None
    kinds = []
    for a in split_args(m.group(2)):
        a = re.sub(r"\bconst\b", "", a).strip()
        if a == "void" or a == "":
            continue
        if a == "float":
            kinds.append("f")
        elif a == "double":
            kinds.append("d")
        elif a.endswith(("*", "&")) and "(" not in a:
            kinds.append("p")
        elif a == "bool":
            kinds.append("b")
        elif a in INT_TYPES or ENUM.match(a):
            kinds.append("i")
        else:
            return None
    return kinds


def statics(headers):
    """The `static` methods of the classes in the sources: 'Class::name'."""
    found = set()
    for root, _, names in os.walk(headers):
        for n in names:
            if not n.endswith(".h"):
                continue
            text = open(os.path.join(root, n), encoding="utf8", errors="replace").read()
            for cls in re.finditer(r"\b(?:class|struct)\s+(\w+)[^;{]*\{", text):
                end = text.find("\n};", cls.end())
                body = text[cls.end(): end if end > 0 else len(text)]
                for m in re.finditer(r"^\s*static\s+[\w:<>*& ]+?\s+\*?&?(\w+)\s*\(", body, re.M):
                    found.add(cls.group(1) + "::" + m.group(1))
    return found


def return_types(headers):
    """{(class or '', name): return type} from the declarations and definitions in the sources (the executables do not say)."""
    found = {}
    rx = re.compile(r"(?m)^\s*(?:static\s+|inline\s+|extern\s+|virtual\s+|constexpr\s+)*([A-Za-z_][\w:<>]*(?:\s+[A-Za-z_]\w*)?(?:\s*[*&])?)\s+(?:(\w+)::)?(\w+)\s*\(")
    for root, _, names in os.walk(headers):
        for n in names:
            if not n.endswith((".h", ".cpp")):
                continue
            text = open(os.path.join(root, n), encoding="utf8", errors="replace").read()
            for m in rx.finditer(text):
                t = re.sub(r"\s+", " ", m.group(1)).strip()
                if t.split()[0] in ("return", "else", "new", "delete", "if", "while", "for", "case", "throw", "using", "typedef"):
                    continue
                found.setdefault((m.group(2) or "", m.group(3)), t)
    return found


def inputs(kinds, rng):
    ints = [-300, -129, -128, -101, -100, -99, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 31, 32, 33, 63, 64,
            65, 99, 100, 101, 127, 128, 129, 255, 256, 257, 359, 360, 361, 999, 1000, 1001, 4095, 4096, 65535, 65536, 0x7FFFFFFF,
            -0x80000000]
    floats = [-1000.0, -10.0, -1.5, -1.0, -0.5, -0.0001, 0.0, 0.0001, 0.25, 0.5, 1.0, 1.5, 2.0, 3.14159, 10.0, 99.9, 100.0, 1000.0, 1e6]
    n_i, n_f = kinds.count("i") + kinds.count("b"), kinds.count("f") + kinds.count("d")
    if n_i == 1 and n_f == 0:
        for v in range(-5, 420):
            yield [v], []
        for v in ints:
            yield [v], []
        return
    if n_i == 0 and n_f == 1:
        for v in floats:
            yield [], [v]
        return
    for _ in range(400):
        yield [rng.choice(ints) for _ in range(n_i)], [rng.choice(floats) for _ in range(n_f)]


def same(a, b, ret, tolerance=2e-6):
    """Do the two results agree, in the register that the return type uses."""
    if ret in ("float", "double"):
        fmt, width = ("<f", 4) if ret == "float" else ("<d", 8)
        x, = struct.unpack(fmt, a[1][:width])
        y, = struct.unpack(fmt, b[1][:width])
        return x == y or (x != x and y != y) or (x == x and y == y and abs(x - y) <= tolerance * max(1.0, abs(x), abs(y)))
    mask = {"bool": 0xFF, "char": 0xFF, "unsigned char": 0xFF, "short": 0xFFFF, "unsigned short": 0xFFFF, "long": 0xFFFFFFFFFFFFFFFF,
            "unsigned long": 0xFFFFFFFFFFFFFFFF}.get(ret, 0xFFFFFFFF)
    return (a[0] & mask) == (b[0] & mask)


def kind_of_return(t):
    t = re.sub(r"(const|static|inline)", "", t).strip()
    if t in ("float", "double"):
        return t
    if t in INT_TYPES or ENUM.match(t):
        return t
    return None


def make_block(rng, index, safe=False):
    """A block of random small numbers; its first 8 bytes point at an empty string (a wxString or std::string given as a reference)."""
    block = bytearray()
    for _ in range(BLOCK // 4):
        roll = rng.random()
        if safe:
            block += struct.pack("<I", rng.choice([0, 0, 1]))  # (only the values that a bool has: 0 and 1)
        elif roll < 0.15:
            block += struct.pack("<I", 0)
        elif roll < 0.8:
            block += struct.pack("<I", rng.randint(0, 8))
        elif roll < 0.9:
            block += struct.pack("<f", rng.choice([0.5, 1.0, 2.5, 10.0, 100.0, -3.0]))
        else:
            block += struct.pack("<I", rng.choice([0xFFFFFFFF, 100, 255, 359, 1000]))
    address = HEAP + index * BLOCK_STRIDE
    if index > 0:
        block[0xE8:0x100] = bytes(0x18)
        block[0:8] = struct.pack("<Q", address + 0x100)
    return address, bytes(block)


def main():
    args = sys.argv[1:]
    headers = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src", "deponia4")
    flt = ""
    only_list = "--list" in args
    methods = "--methods" in args
    trials = 10
    if "--trials" in args:
        i = args.index("--trials")
        trials = int(args[i + 1])
        del args[i:i + 2]
    quiet = "--quiet" in args
    args = [a for a in args if a not in ("--list", "--methods", "--quiet")]
    for opt in ("--headers", "--filter"):
        if opt in args:
            i = args.index(opt)
            if opt == "--headers":
                headers = args[i + 1]
            else:
                flt = args[i + 1]
            del args[i:i + 2]
    if len(args) != 2:
        print(__doc__)
        return 1
    s1, s2 = symbols(args[0]), symbols(args[1])
    common = sorted(set(s1) & set(s2))
    dem = demangle([n for n in common if n.startswith("_Z")])
    static_names = statics(headers)
    returns = return_types(headers)
    todo = []
    skipped_unknown = []
    for n in common:
        d = dem.get(n)
        if not d or d.startswith(("std::", "__gnu", "operator ", "wx", "_", "hb_", "FT_", "SDL_", "lua", "tolua")) or "[clone" in d:
            continue
        kinds = classify(d)
        if kinds is None or len(kinds) > 5 or (not kinds and not methods):
            continue
        head = d.split("(")[0]
        is_method = False
        if "::" in head:
            if head not in static_names:
                if not methods or not re.match(r"^T\w*::\w+$", head):
                    continue
                is_method = True
        elif methods:
            continue
        if not methods and "p" in kinds:
            continue
        if flt and flt not in d:
            continue
        cls, _, name = head.rpartition("::")
        ret = kind_of_return(returns.get((cls.split("::")[-1], name), returns.get(("", name), "")))
        if ret is None:
            if not is_method:
                skipped_unknown.append(d)
                continue
            ret = "memory"  # (a method of a type not known: only what it writes into the blocks is compared)
        if methods and not is_method:
            continue
        todo.append((n, d, kinds, ret, is_method))
    print("candidates: %d" % len(todo))
    if only_list:
        for n, d, k, r, m in todo:
            print(" ", d, "".join(k), "->", r)
        print("return type not known from the sources: %d" % len(skipped_unknown))
        return 0
    im1, im2 = Image(args[0]), Image(args[1])
    rng = random.Random(1)
    ran = same_all = differ = skipped = weak = 0
    skip_reasons = {}
    for n, d, kinds, ret, is_method in todo:
        diffs = []
        count = 0
        trial_no = 0
        failed = None
        arg_sets = list(inputs([k if k != "p" else "i" for k in kinds if k != "p"], rng)) if [k for k in kinds if k != "p"] else [([], [])]
        if is_method or "p" in kinds:
            rng.shuffle(arg_sets)
            arg_sets = arg_sets[:trials]
        for iv, fv in arg_sets:
            iv = list(iv)
            for pos, k in enumerate([k for k in kinds if k in "ib"]):
                if k == "b":
                    iv[pos] &= 1  # (a bool is 0 or 1)
            # the blocks: `this`, then one for each pointer or reference, in the order of the arguments
            blocks = []
            ints = []
            next_int = iter(iv)
            safe = (trial_no % 2 == 0)  # (every other set has blocks of 0 and 1 only: a bool member holding 2 is read differently by GCC -O0 and -O2)
            trial_no += 1
            if is_method:
                blocks.append(make_block(rng, 0, safe))
                ints.append(blocks[0][0])
            for k in kinds:
                if k == "p":
                    blocks.append(make_block(rng, len(blocks), safe))
                    ints.append(blocks[-1][0])
                elif k in "ib":
                    ints.append(next(next_int))
            fl = []
            fi = iter(fv)
            for k in kinds:
                if k in "fd":
                    fl.append((next(fi), k == "d"))
            try:
                r1 = im1.call(s1[n], ints, fl, blocks)
                r2 = im2.call(s2[n], ints, fl, blocks)
            except OSError:
                print("emulator crashed in %s" % d)
                im1, im2 = Image(args[0]), Image(args[1])
                r1 = r2 = None
            if r1 is None or r2 is None:
                failed = "image 1" if r1 is None else "image 2"
                break
            count += 1
            problems = []
            if ret != "memory" and not same((r1["rax"], r1["xmm0"]), (r2["rax"], r2["xmm0"]), ret):
                problems.append("returns %x / %x" % (r1["rax"] & 0xFFFFFFFF, r2["rax"] & 0xFFFFFFFF))
            # (what a std::string member would have written into a block was not written: those blocks cannot be compared)
            for bi, (b1, b2) in enumerate(zip(r1["blocks"], r2["blocks"])):
                if b1 != b2 and not (r1["strings"] or r2["strings"]):
                    at = next(i for i in range(len(b1)) if b1[i] != b2[i]) * 8
                    problems.append("%s differs at +0x%x (%x / %x)" % (["this", "argument"][bi > 0 or not is_method] if is_method else "argument %d" % bi,
                                                                       at, b1[at // 8] & 0xFFFFFFFFFFFFFFFF, b2[at // 8] & 0xFFFFFFFFFFFFFFFF))
                    break
            if r1["trace"] != r2["trace"]:
                problems.append("calls %s / %s" % (r1["trace"][:6], r2["trace"][:6]))
            if problems:
                diffs.append((iv, [f[0] for f in fl], problems, safe))
        if failed or count == 0:
            skipped += 1
            skip_reasons[failed or "none"] = skip_reasons.get(failed or "none", 0) + 1
            continue
        ran += 1
        if diffs and not any(x[3] for x in diffs) and is_method:
            weak += 1
            print("weak      %s: %d of %d inputs differ, only with blocks that hold numbers over 1, e.g. %s" % (d, len(diffs), count, [x[:3] for x in diffs[:1]]))
        elif diffs:
            differ += 1
            print("DIFFERENT %s: %d of %d inputs differ, e.g. %s" % (d, len(diffs), count, [x[:3] for x in diffs[:2]]))
        else:
            same_all += 1
            if not quiet:
                print("same      %s (%d inputs)" % (d, count))
    print("ran %d, the same on every input %d, different %d, different only in blocks with numbers over 1 %d, skipped %d" % (ran, same_all, differ, weak, skipped))
    return 0


sys.exit(main())
