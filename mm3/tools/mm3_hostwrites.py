#!/usr/bin/env python3
"""Cross-check what the hand-written (replayed) host routines write against what the original routines write.

The lockstep replay cannot check a routine whose effect it replays (DOS, files, memory manager, video, sound): whatever the C
host did is simply believed.  What it *can* record is which data-segment bytes each such host wrote.  This tool statically
walks the original routine (and everything it calls, through overlay thunks) and collects the absolute data-segment
addresses it can store to, then compares the two sets:

  * a byte the host wrote that the original never writes  -> the host probably writes the wrong variable
                                                              (e.g. MemFree stored its figure at the wrong offset);
  * a variable the original writes but the host never did -> the host forgot an effect (or a run never exercised it).

Stores through registers (`mov [bx], al`, `rep stosb` ...) cannot be resolved statically; routines that have any are marked
`indirect` and an unexplained host write is then only a hint.

    python3 tools/mm3_hostwrites.py stats1.json [stats2.json ...]     (stats written by `mm3_lockstep.py --stats`)
"""
import argparse
import os
import re
import sys

import capstone
from capstone import x86_const as X

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import asm2c  # noqa: E402
import mm3_lockstep as L  # noqa: E402

DG_LIN = L.DG_LIN


class Walker:
    def __init__(self, image, tr):
        self.img = image                      # bytes of the program image, linear 10000h upward
        self.tr = tr
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
        self.md.detail = True
        self.stubs = {s * 16: i for i, s in enumerate(L.STUB_SEGMENTS)}

    def byte(self, lin):
        o = lin - 0x10000
        return self.img[o] if 0 <= o < len(self.img) else 0

    def resolve_thunk(self, lin):
        """`int 3Fh, dw offset` stub thunks -> the overlay routine"""
        for base, k in self.stubs.items():
            if base <= lin < base + 0x1000 and self.byte(lin) == 0xCD and self.byte(lin + 1) == 0x3F:
                off = self.byte(lin + 2) | (self.byte(lin + 3) << 8)
                return L.OVERLAY_SEGMENTS[k] * 16 + off
        return lin

    def walk(self, entry, limit=4000):
        """-> (set of (offset, size) absolute data segment stores, indirect-store flag, set of visited routine entries)"""
        writes, indirect = set(), False
        todo, seen, calls = [entry], set(), set()
        steps = 0
        while todo and steps < 200000:
            lin = self.resolve_thunk(todo.pop())
            if lin in seen or lin < 0x10000 or lin >= 0x10000 + len(self.img):
                continue
            while True:
                if lin in seen:
                    break
                seen.add(lin)
                code = bytes(self.img[lin - 0x10000:lin - 0x10000 + 16])
                ins = next(self.md.disasm(code, lin), None)
                steps += 1
                if ins is None:
                    break
                for op in ins.operands:
                    if op.type == X.X86_OP_MEM and (op.access & capstone.CS_AC_WRITE):
                        seg = ins.reg_name(op.mem.segment) if op.mem.segment else "ds"
                        if ins.mnemonic.startswith(("stos", "movs")):
                            indirect = True
                        elif seg in ("ds", "ss") and not op.mem.base and not op.mem.index:
                            writes.add((op.mem.disp & 0xFFFF, op.size))
                        elif seg in ("ds", "ss", "es", "cs"):
                            indirect = True
                if ins.mnemonic.startswith(("stos", "movs")) and ins.mnemonic not in ("movsx",):
                    indirect = True
                mnem = ins.mnemonic
                if mnem in ("ret", "retf", "retn", "lret", "iret"):
                    break
                if mnem == "call" or mnem.startswith("j") or mnem == "loop":
                    tgt = None
                    if ins.op_str.startswith("0x"):
                        tgt = int(ins.op_str, 16)
                        if mnem in ("call", "jmp") and ":" not in ins.op_str:
                            pass
                    elif ":" in ins.op_str and re.match(r"^0x[0-9a-f]+:0x[0-9a-f]+$", ins.op_str):
                        seg_, off_ = ins.op_str.split(":")
                        tgt = int(seg_, 16) * 16 + int(off_, 16)
                    if tgt is not None:
                        if mnem == "call":
                            calls.add(tgt)
                            todo.append(tgt)
                        else:
                            todo.append(tgt)
                    if mnem == "jmp":
                        break
                lin += ins.size
        return writes, indirect, seen


def merge_ranges(offs):
    out = []
    for o in sorted(offs):
        if out and o <= out[-1][1] + 1:
            out[-1][1] = max(out[-1][1], o)
        else:
            out.append([o, o])
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("stats", nargs="+")
    ap.add_argument("--asm", default=os.path.join(HERE, "..", "mm3.asm"))
    ap.add_argument("--idc", default=os.path.join(HERE, "..", "mm3.idc"))
    ap.add_argument("--image", default=os.path.join(HERE, "..", "data", "IMAGE.BIN"))
    ap.add_argument("--stack-from", type=lambda s: int(s, 0), default=0xF200, help="data segment offset where the stack region starts")
    a = ap.parse_args()
    import json
    host_writes = {}
    for p in a.stats:
        for n, offs in json.load(open(p)).get("writes", {}).items():
            host_writes.setdefault(n, set()).update(offs)
    tr = asm2c.Translator(a.asm, a.idc)
    img = open(a.image, "rb").read()
    w = Walker(img, tr)
    dgsyms = sorted((v - DG_LIN, k) for k, v in tr.names.items() if DG_LIN <= v < DG_LIN + 0x10000)

    def sym(off):
        best = None
        for o, n in dgsyms:
            if o <= off:
                best = (o, n)
            else:
                break
        return "%s%+d" % (best[1], off - best[0]) if best and off - best[0] else (best[1] if best else "?")

    bad = 0
    for name in sorted(host_writes):
        if name.startswith("vdrv_"):
            print("%-26s (video module: its code is loaded at run time, not part of the image)" % name)
            continue
        host_writes[name] = set(o for o in host_writes[name] if o < a.stack_from)   # (the stack is whatever the callers' frames hold)
        if not host_writes[name]:
            continue
        lin = tr.func_linear(L.canon(name))
        if lin is None:
            print("%-26s (no address)" % name)
            continue
        stores, indirect, seen = w.walk(lin)
        covered = set()
        for off, size in stores:
            covered.update(range(off, off + size))
        hw = host_writes[name]
        unexplained = sorted(o for o in hw if o not in covered)
        verdict = "ok"
        if unexplained:
            verdict = "CHECK" if not indirect else "indirect"
            if not indirect:
                bad += 1
        print("%-26s host wrote %5d byte(s), original has %3d direct store(s)%s -> %s" % (
            name, len(hw), len(stores), " + indirect stores" if indirect else "", verdict))
        for lo, hi in merge_ranges(unexplained)[:6]:
            print("      host wrote %04X-%04X (%s) which the original never stores to directly" % (lo, hi, sym(lo)))
        missing = sorted(set(o for o, _ in stores) - hw)
        if missing:
            print("      original stores but the host never wrote: %s" % ", ".join("%04X %s" % (o, sym(o)) for o in missing[:8]))
    print("\n%d host routine(s) wrote data the original routine never stores to (no indirect stores to explain it)" % bad)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
