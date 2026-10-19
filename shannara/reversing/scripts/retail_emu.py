"""
Runs functions of the real retail SHANNARA.EXE under the unicorn x86 emulator, straight from the executable file (no IDA needed).
The LE objects are mapped at their link addresses (code 0x10000, data 0xA0000) so no relocation is required; uninitialised data is
zero. Watcom register calling convention: arguments in eax, edx, ebx, ecx.

    from retail_emu import RetailEmu
    e = RetailEmu("game/SHANNARA.EXE")
    e.call(0x1A200, eax=88)                      # object_get_location?(88)
    e.hook(0x10F70, my_python_function)          # replace a function (receives the emulator, returns eax)

Used for differential testing of the C++ world model (port/tests/world_fuzz.py).
"""
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, \
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP

STACK = 0x900000
RET = 0x800000


class LeImage:
    """DOS/4GW LE objects of an executable (also usable on its own to read static data)."""

    def __init__(self, path):
        d = self.d = open(path, "rb").read()
        hdr = stub = None
        for o in range(0, len(d) - 0x40):
            if d[o:o + 2] == b"MZ":
                h = o + struct.unpack_from("<I", d, o + 0x3C)[0]
                if h + 4 < len(d) and d[h:h + 4] == b"LE\0\0":
                    stub, hdr = o, h
                    break
        if hdr is None:
            raise ValueError("no LE header")
        u = lambda off: struct.unpack_from("<I", d, hdr + off)[0]
        self.page = u(0x28)
        pages = u(0x14)
        self.eip = u(0x1C)
        self.data_off = stub + u(0x80)
        self.objects = []
        for i in range(u(0x44)):
            size, base, flags, first, npages, _ = struct.unpack_from("<6I", d, hdr + u(0x40) + 24 * i)
            self.objects.append((base, size, first, npages))
        self.page_map = []
        for i in range(pages):
            e = u(0x48) + 4 * i
            b = d[hdr + e: hdr + e + 3]
            self.page_map.append((b[0] << 16) | (b[1] << 8) | b[2])
        self.hdr = hdr
        self.fixup_page_tab = hdr + u(0x68)
        self.fixup_rec_tab = hdr + u(0x6C)
        self.npages = pages

    def object_bytes(self, k, fixups=True):
        base, size, first, npages = self.objects[k]
        out = bytearray(max(size, npages * self.page))
        for p in range(npages):
            off = self.data_off + (self.page_map[first - 1 + p] - 1) * self.page
            chunk = self.d[off: off + self.page]
            out[p * self.page: p * self.page + len(chunk)] = chunk
        if fixups:
            for p in range(npages):
                self._apply_page_fixups(out, p * self.page, first - 1 + p, base + p * self.page)
        return base, bytes(out)

    def fixup_sites(self):
        """every relocated 32-bit field of the image as (linear address of the field, kind, resolved target value): operands of the decoded
        instructions that coincide with one of these were data/code pointers (relocated by the loader); all other constants stay as they are"""
        self._collect = []
        for k in range(len(self.objects)):
            self.object_bytes(k, True)
        out, self._collect = self._collect, None
        return out

    _collect = None

    def _apply_page_fixups(self, out, page_off, logical_page, page_addr):
        """internal-reference fixups of one page (the only kind this executable uses): value = base(target object) + target offset"""
        d = self.d
        a = struct.unpack_from("<I", d, self.fixup_page_tab + 4 * logical_page)[0]
        b = struct.unpack_from("<I", d, self.fixup_page_tab + 4 * (logical_page + 1))[0]
        p = self.fixup_rec_tab + a
        end = self.fixup_rec_tab + b
        while p < end:
            src, flags = d[p], d[p + 1]
            p += 2
            if src & 0x20:
                cnt = d[p]
                p += 1
                srcoffs = None
            else:
                srcoffs = [struct.unpack_from("<h", d, p)[0]]
                p += 2
                cnt = 0
            if flags & 3 != 0:
                raise ValueError("unsupported fixup target kind %d" % (flags & 3))
            if flags & 0x40:
                obj = struct.unpack_from("<H", d, p)[0]
                p += 2
            else:
                obj = d[p]
                p += 1
            kind = src & 0xF
            tgt = 0
            if kind != 2:
                if flags & 0x10:
                    tgt = struct.unpack_from("<I", d, p)[0]
                    p += 4
                else:
                    tgt = struct.unpack_from("<H", d, p)[0]
                    p += 2
            if flags & 4:
                if flags & 0x20:
                    tgt += struct.unpack_from("<I", d, p)[0]
                    p += 4
                else:
                    tgt += struct.unpack_from("<H", d, p)[0]
                    p += 2
            if srcoffs is None:
                srcoffs = list(struct.unpack_from("<%dh" % cnt, d, p))
                p += 2 * cnt
            value = self.objects[obj - 1][0] + tgt
            for so in srcoffs:
                o = page_off + so
                if self._collect is not None:
                    self._collect.append((page_addr + so, kind, value & 0xFFFFFFFF))
                if kind == 7:
                    if 0 <= o and o + 4 <= len(out):
                        out[o:o + 4] = struct.pack("<I", value & 0xFFFFFFFF)
                elif kind == 8:
                    if 0 <= o and o + 4 <= len(out):
                        out[o:o + 4] = struct.pack("<I", (value - (page_addr + so + 4)) & 0xFFFFFFFF)
                elif kind == 5:
                    if 0 <= o and o + 2 <= len(out):
                        out[o:o + 2] = struct.pack("<H", value & 0xFFFF)
                elif kind in (2, 3, 6, 0):
                    pass   # selector / 16:16 / 16:32 pointers and byte fixups do not occur in flat DOS/4GW code with these tables


class RetailEmu:
    def __init__(self, exe_path, max_insns=2_000_000):
        self.img = LeImage(exe_path)
        self.max_insns = max_insns
        self.hooks = {}
        self.uc = uc = Uc(UC_ARCH_X86, UC_MODE_32)
        for k in range(len(self.img.objects)):
            base, data = self.img.object_bytes(k)
            size = (len(data) + 0xFFF) & ~0xFFF
            uc.mem_map(base, size + 0x1000)
            uc.mem_write(base, data)
        uc.mem_map(STACK - 0x20000, 0x40000)
        uc.mem_map(RET, 0x1000)
        uc.mem_write(RET, b"\xf4")
        uc.hook_add(UC_HOOK_CODE, self._code_hook)

    # -- memory helpers
    def r8(self, a): return self.uc.mem_read(a, 1)[0]
    def r16(self, a): return struct.unpack("<H", self.uc.mem_read(a, 2))[0]
    def r32(self, a): return struct.unpack("<I", self.uc.mem_read(a, 4))[0]
    def rb(self, a, n): return bytes(self.uc.mem_read(a, n))
    def w8(self, a, v): self.uc.mem_write(a, bytes([v & 0xFF]))
    def w16(self, a, v): self.uc.mem_write(a, struct.pack("<H", v & 0xFFFF))
    def w32(self, a, v): self.uc.mem_write(a, struct.pack("<I", v & 0xFFFFFFFF))
    def reg(self, r): return self.uc.reg_read(r)

    def hook(self, addr, fn):
        """fn(emu) runs instead of the function at `addr`; its return value goes to eax; the function returns normally"""
        self.hooks[addr] = fn

    def _code_hook(self, uc, address, size, user):
        fn = self.hooks.get(address)
        if fn is None:
            return
        self.pop_extra = 0          # a hook sets this to the callee-cleaned argument bytes (`retn N`) that must go too
        eax = fn(self)
        uc.reg_write(UC_X86_REG_EAX, (eax or 0) & 0xFFFFFFFF)
        sp = uc.reg_read(UC_X86_REG_ESP)
        ret = struct.unpack("<I", uc.mem_read(sp, 4))[0]
        uc.reg_write(UC_X86_REG_ESP, sp + 4 + self.pop_extra)
        uc.reg_write(UC_X86_REG_EIP, ret)

    def call(self, addr, eax=0, edx=0, ebx=0, ecx=0, stack=()):
        """calls a function with register arguments (plus optional cdecl-style stack words); returns eax (u32)"""
        uc = self.uc
        sp = STACK
        for v in reversed(stack):
            sp -= 4
            uc.mem_write(sp, struct.pack("<I", v & 0xFFFFFFFF))
        sp -= 4
        uc.mem_write(sp, struct.pack("<I", RET))
        uc.reg_write(UC_X86_REG_ESP, sp)
        uc.reg_write(UC_X86_REG_EAX, eax & 0xFFFFFFFF)
        uc.reg_write(UC_X86_REG_EDX, edx & 0xFFFFFFFF)
        uc.reg_write(UC_X86_REG_EBX, ebx & 0xFFFFFFFF)
        uc.reg_write(UC_X86_REG_ECX, ecx & 0xFFFFFFFF)
        for r in (UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            uc.reg_write(r, 0)
        ctx = uc.context_save()
        try:
            uc.emu_start(addr, RET, count=self.max_insns)
        except Exception:
            uc.context_restore(ctx)     # leave the CPU usable after an emulation fault (memory is not rolled back)
            raise
        if uc.reg_read(UC_X86_REG_EIP) != RET:
            raise RuntimeError("function at 0x%X did not return (instruction budget exhausted?)" % addr)
        return uc.reg_read(UC_X86_REG_EAX)
