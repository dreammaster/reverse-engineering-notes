"""
Validates the Q *container* layer (header + chunk types 0-7, q_open/q_load/q_expand) against the real demo code.

Synthetic .Q files are built from random valid block streams (q_fuzz.Gen), the file I/O and allocator routines of the
demo are replaced by Python hooks, and q_open / q_load / q_expand run under unicorn.  A small Python player that
follows analysis/q_codec_spec.md must produce the same video buffer after every frame.

    python q_container_test.py <qdump dir> [files] [seed]
"""
import random
import struct
import sys

from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *

from q_decode_ref import *
from q_emulate import QEmu, VIDEO, VIDEO_SIZE, STACK, RETADDR
from q_fuzz import Gen

FILE_BASE = 0x1000
HEAP = 0xC80000          # mapped by this script
HEAP_SIZE = 0x200000
BUF = HEAP + 0x100000    # q buffer (0x13C06 bytes)
PAL = HEAP + 0x1F0000


def build_file(rng, tables, W, H, nframes, fps=10, audio=11):
    """returns (bytes, expected list of (kind, params)) - the file as the encoder would write it"""
    hdr = bytearray(0x1A)
    struct.pack_into("<H", hdr, 0, 0x6839)
    struct.pack_into("<H", hdr, 4, W)
    struct.pack_into("<H", hdr, 6, H)
    hdr[8] = 4
    hdr[9] = 4
    struct.pack_into("<H", hdr, 0xA, nframes)
    hdr[0x10] = fps
    struct.pack_into("<I", hdr, 0x16, fps * audio)
    chunks = []          # (type, payload, trailer)

    def chunk(t, payload, trailer=b"", size=None):
        chunks.append((t, bytes(payload), trailer, len(payload) if size is None else size))
    qmap = bytes(rng.randrange(256) for _ in range(0x100))
    chunk(1, bytes(rng.randrange(64) for _ in range(0x300)))
    chunk(5, qmap)
    # a scratch decoder only to keep the generator's choices valid
    have_comp = False
    raw = False
    cur_win = (0, 0, W - 1, H)      # a type-2 frame reuses the last window (qloop values persist)
    for f in range(nframes):
        if rng.random() < 0.25:
            chunk(6, b"")
            have_comp = True
        if rng.random() < 0.1:
            chunk(7, b"")
            raw = True
        if rng.random() < 0.3:
            chunk(0, bytes(rng.randrange(256) for _ in range(audio)))
        window = rng.random() < 0.4
        if window:
            min_col = rng.randrange(0, W)
            max_col = rng.randrange(0, W - min_col)
            min_row = rng.randrange(0, H)
            max_row = rng.randrange(1, H - min_row + 1)
            cur_win = (min_col, min_row, max_col, max_row)
        min_col, min_row, max_col, max_row = cur_win
        v = bytearray(VIDEO_SIZE)
        dec = QDecoder(tables, W, H, qmap, v, composite=bytearray(VIDEO_SIZE))
        stream = bytearray()
        dec.s = stream
        dec.p = 0
        dec.idx = min_row * W + min_col
        dec.cols, dec.rows = max_col, max_row
        dec.start = dec.dst = dec.rowstart = dec.baddr(dec.idx)
        dec.row_i = 0
        dec.max_col, dec.min_col = max_col, min_col
        dec.state = T0
        dec.done = False
        dec.raw_mode = raw
        # the raw flag and stale window are properties of the player, so keep generation conservative:
        gen = Gen(dec, rng, has_comp=have_comp)
        guard = 0
        while not dec.done and guard < 3000:
            guard += 1
            stream += gen.choose()
            dec.step()
        payload = bytes(rng.randrange(256) for _ in range(audio)) + bytes(stream)
        if window:
            chunk(rng.choice((3, 4)), payload, struct.pack("<4H", min_col, min_row, max_col, max_row), len(stream))
        else:
            chunk(2, payload, b"", len(stream))
    chunk(0xFFFF, b"")
    out = bytearray(hdr)
    # every chunk is: prefix(6) payload trailer ; the prefix of chunk i+1 directly follows chunk i
    for (t, payload, trailer, size) in chunks:
        out += struct.pack("<HI", t, size) + payload + trailer
    return bytes(out), qmap


class Player:
    """Python implementation of the container rules from the spec."""

    def __init__(self, tables, W, H, data):
        self.t, self.W, self.H = tables, W, H
        self.d = data
        self.v = bytearray(VIDEO_SIZE)
        self.comp = None
        self.qmap = bytes(0x100)
        self.raw = False
        self.win = (0, 0, W - 1, H)
        self.pos = 0x1A
        self.audio = struct.unpack_from("<I", data, 0x16)[0] // data[0x10]

    def next_frame(self):
        d = self.d
        while True:
            t, size = struct.unpack_from("<HI", d, self.pos)
            body = self.pos + 6
            if t == 0xFFFF:
                return None
            if t in (2, 3, 4):
                size += self.audio          # the size field does not count the audio bytes
            payload = d[body: body + size]
            self.pos = body + size
            if t == 0:
                continue
            if t == 1:
                continue
            if t == 5:
                self.qmap = bytes(payload) + bytes(0x100 - len(payload))
                continue
            if t == 6:
                n = self.W * 4 * self.H * 4
                self.comp = bytearray(self.v[:n]) + bytearray(0x10000)
                continue
            if t == 7:
                self.raw = True
                continue
            if t in (3, 4):
                self.win = struct.unpack_from("<4H", d, self.pos)
                self.pos += 8
            stream = payload[self.audio:]
            dec = QDecoder(self.t, self.W, self.H, self.qmap, self.v, composite=self.comp)
            dec.raw_mode = self.raw
            dec.decode(bytes(stream) + bytes(64), *self.win)
            return True


def run(dump_dir, nfiles=10, seed=1, W=160, H=4):
    tables = load_tables(dump_dir)
    rng = random.Random(seed)
    bad = 0
    for fi in range(nfiles):
        data, qmap = build_file(rng, tables, W, H, rng.randrange(2, 7))
        e = QEmu(dump_dir, W, H)
        uc = e.uc
        n = e.names
        uc.mem_map(HEAP, HEAP_SIZE)
        uc.mem_write(FILE_BASE + 0x100000, b"")  # (file data kept in Python)
        state = {"pos": 0, "heap": HEAP}

        def ret(u, val=None):
            if val is not None:
                u.reg_write(UC_X86_REG_EAX, val)
            sp = u.reg_read(UC_X86_REG_ESP)
            eip = struct.unpack("<I", bytes(u.mem_read(sp, 4)))[0]
            u.reg_write(UC_X86_REG_ESP, sp + 4)
            u.reg_write(UC_X86_REG_EIP, eip)

        def hook(u, addr, size, ud):
            if addr == n["fp_open"]:
                state["pos"] = 0
                ret(u, 1)
            elif addr == n["fp_read"]:
                dest, cnt = u.reg_read(UC_X86_REG_EDX), u.reg_read(UC_X86_REG_EBX)
                chunk = data[state["pos"]: state["pos"] + cnt]
                u.mem_write(dest, chunk + bytes(cnt - len(chunk)))
                state["pos"] += cnt
                ret(u, cnt)
            elif addr == n["fp_seek"]:
                state["pos"] = u.reg_read(UC_X86_REG_EDX)
                ret(u, 0)
            elif addr in (n["fp_close"], n["fp_set_bufsiz"], n["kill_pointer"]):
                ret(u, 0)
            elif addr == n["new_pointer"]:
                sz = u.reg_read(UC_X86_REG_EAX)
                p = state["heap"]
                state["heap"] += (sz + 15) & ~15
                ret(u, p)
        uc.hook_add(UC_HOOK_CODE, hook, begin=0x157000, end=0x180000)

        def call(addr, **regs):
            for r, v in regs.items():
                uc.reg_write(getattr(__import__("unicorn.x86_const", fromlist=["x"]), "UC_X86_REG_" + r.upper()), v)
            sp = STACK
            uc.mem_write(sp - 4, struct.pack("<I", RETADDR))
            uc.reg_write(UC_X86_REG_ESP, sp - 4)
            try:
                uc.emu_start(addr, RETADDR, count=5_000_000)
            except Exception as ex:
                print('EMU ERROR', ex, 'eip', hex(uc.reg_read(UC_X86_REG_EIP)), 'edx', hex(uc.reg_read(UC_X86_REG_EDX)), 'esi', hex(uc.reg_read(UC_X86_REG_ESI)), 'edi', hex(uc.reg_read(UC_X86_REG_EDI)), 'ebx', hex(uc.reg_read(UC_X86_REG_EBX)), 'ebp', hex(uc.reg_read(UC_X86_REG_EBP)))
                print('qloop2', hex(struct.unpack('<I', bytes(uc.mem_read(n['_qloop2'],4)))[0]), 'qbuffer', hex(struct.unpack('<I', bytes(uc.mem_read(n['_qbuffer'],4)))[0]), 'qfp', hex(struct.unpack('<I', bytes(uc.mem_read(n['_q_fp'],4)))[0]))
                raise
            return uc.reg_read(UC_X86_REG_EAX)

        e.w32(n["_qbuffer"], BUF)
        e.w32(n["_qbuffer_orig"], BUF)
        e.w32(n["_qpalette"], PAL)
        e.w32(n["_video_buffer"], VIDEO)
        e.w32(n["_voice_supported"], 0)
        r = call(n["q_open"], eax=0x1234, edx=0)
        if r != 1:
            print("q_open failed", r)
            bad += 1
            continue
        player = Player(tables, W, H, data)
        frame = 0
        while True:
            call(n["q_load"])
            f = player.next_frame()
            if f is None:
                break
            call(n["q_expand"])
            got = bytes(uc.mem_read(VIDEO, H * ROW))
            ref = bytes(player.v[:H * ROW])
            if got != ref:
                diffs = [i for i in range(len(ref)) if ref[i] != got[i]]
                print("file", fi, "frame", frame, "MISMATCH", len(diffs), "bytes, first", diffs[:3])
                bad += 1
                break
            frame += 1
        else:
            pass
    print("done, files with mismatches:", bad, "of", nfiles)


if __name__ == "__main__":
    run(sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 10, int(sys.argv[3]) if len(sys.argv) > 3 else 1)
