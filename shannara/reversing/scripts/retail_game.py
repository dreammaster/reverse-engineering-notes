"""
A headless "oracle" for the retail game: the real SHANNARA.EXE logic (verb dispatch, object handlers, room changes, flags) runs
under unicorn with every UI / audio / graphics / file routine stubbed out and the game text captured.  It lets you issue commands
and see the exact messages and state changes the original produces, without a display or sound card, and serves as the reference
for re-implementing handlers.

    from retail_game import Game
    g = Game("game/SHANNARA.EXE", "game/SHANSTR.DAT")
    g.command("Look")                  # verb by name (see retail_verb_handlers.tsv), optional dobj / iobj object ids
    print(g.text())                    # text printed by that command

Mechanism: all functions of the engine/library region (>= 0x75000) are replaced by no-op stubs except a whitelist of pure C runtime
routines; `wprintf`/`wputchar` are replaced by Python implementations that expand the format (a first argument >= 0xF0000000 is a
SHANSTR message id) and append to the transcript.  Game-layer UI helpers are stubbed one by one as they are discovered (STUB_GAME).
"""
import csv
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
from retail_emu import RetailEmu   # noqa: E402
import shanstr                      # noqa: E402
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX   # noqa: E402

ANALYSIS = os.path.join(ROOT, "reversing", "analysis")

CRT_KEEP = {"memmove_", "memset_", "memcpy_", "strcmp_", "stricmp_", "sprintf_", "vsprintf_", "strlen_", "strcpy_", "strncpy_", "strncmp_",
            "__STK", "__prtf_", "getprintspecs_", "evalflags_", "fmt4hex_", "FixedPoint_Format_", "far_strlen_", "far_other_strlen_",
            "strupr_", "strlwr_", "toupper_", "tolower_", "__CHK", "__fmemmove", "_fmemmove", "strstr_", "memicmp_"}
STUB_GAME = set()      # game-layer addresses (hex ints) forced to no-op stubs, filled in as needed
# game-layer routines that only drive the UI / audio / timing and have no game-state effect (stubbed by name)
STUB_NAMES = {"delay?", "delay_animate?", "wait_ticks?", "show_dialog?", "make_voice?", "make_sound", "refresh_room?", "send_event?",
              "send_event_code?", "dispatch_event?", "play_music_entry?", "set_room_music?", "journal_render_page?", "setup_pic_regions?",
              "disable_pic_regions?", "load_pic_sequence?", "play_cutscene?", "describe_contents_ui?", "sub_73D1C", "sub_101A0", "sub_22D50", "sub_10FA0"}
VERB_FLAGS = {}


def load_tsv(name):
    with open(os.path.join(ANALYSIS, name), encoding="utf-8") as f:
        return list(csv.DictReader(f, delimiter="\t"))


class Game(RetailEmu):
    def __init__(self, exe, shanstr_path, keep_extra=()):
        super().__init__(exe, max_insns=5_000_000)
        self.str = shanstr.ShanStr(open(shanstr_path, "rb").read())
        self.out = []
        self.wlog = []      # (caller, first argument, format) for every wprintf
        self.heap = 0xC00000
        self.uc.mem_map(self.heap, 0x400000)
        self.calls = []
        self.callers = []   # (stubbed function, calling function+offset) for every stub call
        fmap = load_tsv("retail_function_map.tsv")
        self.names = {int(r["address"], 16): r["name"] for r in fmap}
        self.byname = {r["name"]: int(r["address"], 16) for r in fmap}
        self.retn = self._retn_map(os.path.join(ROOT, "Shannara.asm"))
        keep = set(CRT_KEEP) | set(keep_extra)
        for r in fmap:
            a = int(r["address"], 16)
            if a >= 0x75000 and r["name"] not in keep:
                self.hook(a, self._stub)
        for a in STUB_GAME:
            self.hook(a, self._stub)
        for n in STUB_NAMES:
            if n in self.byname:
                self.hook(self.byname[n], self._stub)
        self.hook(self.byname["wprintf"], self._wprintf)
        self.hook(self.byname["wputchar"], self._wputchar)
        self.hook(self.byname["sprintf_"], self._sprintf)   # the C runtime's __prtf_ uses segment-register tricks that unicorn rejects
        self.hook(self.byname["new_pointer"], self._alloc)
        # what load_object_dat? does at startup: the name/description pool
        d = open(os.path.join(os.path.dirname(shanstr_path), "OBJECT.DAT"), "rb").read()
        n = struct.unpack_from("<H", d, 0)[0]
        self.uc.mem_write(0xCD9D0, d[2:2 + n])
        self.verbs = {r["name"]: int(r["verb"]) for r in load_tsv("retail_verb_handlers.tsv") if r["name"]}
        self.verb_words = {k.lower(): v for k, v in self.verbs.items()}

    @staticmethod
    def _retn_map(asm):
        """function name -> bytes its `retn N` pops (callee-cleaned stack arguments); stubs must pop the same amount"""
        import re
        cache = os.path.join(ROOT, "game_out", "retn_map.txt")
        if os.path.exists(cache) and os.path.getmtime(cache) > os.path.getmtime(asm):
            return {k: int(v) for k, v in (l.split() for l in open(cache))}
        m = {}
        cur = None
        for line in open(asm, encoding="latin-1"):
            mm = re.match(r"^(\S+)\s+proc", line)
            if mm:
                cur = mm.group(1)
                continue
            if cur and cur not in m:
                r = re.match(r"\s+retn\s+([0-9A-Fa-f]+)h?\s*(;.*)?$", line)
                if r:
                    v = r.group(1)
                    m[cur] = int(v, 16) if re.search("[A-Fa-f]", v) or line.rstrip().split(";")[0].rstrip().endswith("h") else int(v)
        with open(cache, "w") as f:
            f.write("".join("%s %d\n" % kv for kv in m.items()))
        return m

    # ---- stubs -----------------------------------------------------------------------------------------------------------
    def _stub(self, emu):
        name = self.names.get(self._pc, "?")
        self.calls.append(name)
        self.callers.append((name, self._func_at(self.r32(self.reg(UC_X86_REG_ESP)))))
        n = self.retn.get(name, 0)
        if n:       # callee-popped arguments: the hook machinery pops the return address, we pop the arguments below it
            from unicorn.x86_const import UC_X86_REG_ESP as SP
            self.uc.reg_write(SP, self.reg(SP) + n)
        return 0

    def _alloc(self, emu):
        size = emu.reg(UC_X86_REG_EAX) & 0xFFFFFFFF
        p = self.heap
        self.heap += (size + 15) & ~15
        return p

    def _stack(self, i):
        return self.r32((self.reg(UC_X86_REG_ESP) + 4 + 4 * i) & 0xFFFFFFFF)

    def _cstr(self, a):
        s = bytearray()
        while len(s) < 4096:
            try:
                c = self.r8(a + len(s))
            except Exception:
                return b"(null)" if not s else bytes(s)   # NULL / unmapped %s argument
            if c == 0:
                break
            s.append(c)
        return bytes(s)

    def _message(self, v):
        """a wprintf first argument: a pointer to a C string, or a SHANSTR id tagged 0xF0 in the top byte"""
        if v >= 0xF0000000:
            blk, idx = (v >> 16) & 0xFF, v & 0xFFFF
            try:
                return self.str.string(blk, idx).rstrip(b"\0")
            except Exception:
                return b"<bad message id %08x>" % v
        return self._cstr(v)

    def _format(self, fmt, argi):
        out = bytearray()
        i = 0
        while i < len(fmt):
            c = fmt[i]
            if c != 0x25:
                out.append(c)
                i += 1
                continue
            i += 1
            while i < len(fmt) and chr(fmt[i]) in "-+ #0123456789.l":
                i += 1
            t = chr(fmt[i]) if i < len(fmt) else "%"
            i += 1
            if t == "%":
                out.append(0x25)
            elif t == "s":
                out += self._message(self._stack(argi))
                argi += 1
            elif t in "du":
                v = self._stack(argi)
                argi += 1
                out += str(v - (1 << 32) if t == "d" and v >= 1 << 31 else v).encode()
            elif t == "c":
                out.append(self._stack(argi) & 0xFF)
                argi += 1
            else:
                argi += 1
        return bytes(out)

    def _wprintf(self, emu):
        first = self._stack(0)
        fmt = self._message(first)
        self.wlog.append((self._func_at(self.r32(self.reg(UC_X86_REG_ESP))), first, fmt))
        self.out.append(self._format(fmt, 1))
        return 0

    def _func_at(self, a):
        import bisect
        if not hasattr(self, "_addrs"):
            self._addrs = sorted(self.names)
        i = bisect.bisect_right(self._addrs, a) - 1
        return "%s+0x%X" % (self.names[self._addrs[i]], a - self._addrs[i]) if i >= 0 else hex(a)

    def _sprintf(self, emu):
        """cdecl sprintf_(buf, fmt, args...) -> length"""
        buf = self._stack(0)
        out = self._format(self._message(self._stack(1)), 2)
        self.uc.mem_write(buf, out + bytes(1))
        return len(out)

    def _wputchar(self, emu):
        self.out.append(bytes([emu.reg(UC_X86_REG_EAX) & 0xFF]))
        return 0

    # ---- interface -------------------------------------------------------------------------------------------------------
    @property
    def _pc(self):
        from unicorn.x86_const import UC_X86_REG_EIP
        return self.reg(UC_X86_REG_EIP)

    def text(self, clear=True):
        t = b"".join(self.out).decode("latin-1").replace("\x0b", "\n").replace("\r", "\n")
        if clear:
            self.out = []
        return t

    def run_command(self, verb, dobj=0, iobj=0):
        """the original run_command?(verb=eax, dobj=edx, iobj=ebx) (0x1B7B0); returns its result"""
        return self.call(self.byname["run_command?"], eax=verb, edx=dobj, ebx=iobj) & 0xFFFFFFFF

    def command(self, verb, dobj=0, iobj=0, actor=3):
        """issue a command as the UI would: `actor` is the acting character (default 3, Jak)"""
        v = self.verb_words[verb.lower()] if isinstance(verb, str) else verb
        self.w16(0xCD92C, actor)
        self.w16(0xADE98, actor)
        return self.run_command(v, dobj, iobj)

    def where(self):
        """(function name, eip, plausible call chain) at the point the emulation stopped - for diagnosing crashes"""
        import bisect
        addrs = sorted(self.names)
        def fn(a):
            i = bisect.bisect_right(addrs, a) - 1
            return "%s+0x%X" % (self.names[addrs[i]], a - addrs[i]) if i >= 0 else hex(a)
        eip = self._pc
        sp = self.reg(UC_X86_REG_ESP)
        chain = []
        for k in range(0, 256):
            try:
                v = self.r32(sp + 4 * k)
            except Exception:
                break
            if 0x10000 <= v < 0x9D000 and len(chain) < 8:
                chain.append(fn(v))
        return fn(eip), hex(eip), chain

    # ---- snapshots (explore alternatives from the same state) --------------------------------------------------------------
    SNAP_RANGES = ((0x10000, 0x9D000), (0xA0000, 0xF0000))   # code object (it holds tables) and data object + BSS

    def snapshot(self):
        mem = [(a, bytes(self.uc.mem_read(a, b - a))) for a, b in self.SNAP_RANGES]
        heap = (0xC00000, bytes(self.uc.mem_read(0xC00000, max(self.heap - 0xC00000, 1))), self.heap)
        return (mem, heap, len(self.out), len(self.wlog))

    def restore(self, snap):
        mem, heap, nout, nlog = snap
        for a, data in mem:
            self.uc.mem_write(a, data)
        self.uc.mem_write(heap[0], heap[1])
        self.heap = heap[2]
        del self.out[nout:]
        del self.wlog[nlog:]
