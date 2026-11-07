#!/usr/bin/env python3
"""
ACL virtual machine (work in progress): the opcodes whose effect is purely on script state are implemented and verified against the
original handlers (tools/verify_acl_ops.py); every other opcode is recorded in `unsupported` and skipped.
State: things = {id: bytearray(16)} (position records in the game's RAM layout: [3] flags, [15] flags2, [10] room), vars (ds:611+n), the condition
counter `c605` (ds:605: tests increment it on success and zero it on failure), score (ds:24C).   docs/formats.md 6.7
"""
import acl


class VM:
    def __init__(self, things, cur=0, room=0):
        self.things = things           # id -> bytearray(16)
        self.cur = cur                 # id of the current object (ds:9C)
        self.room = room               # ds:232
        self.vars = bytearray(0x40)
        self.c605 = 0
        self.score = 0
        self.disarm = False
        self.unsupported = []
        self.messages = []
        self.selected = 0              # ds:27A, the thing id of the currently selected / carried item
        self.reg = 0                   # ds:704, the 'random / variable' register
        self.rand = None               # callable(lo, hi) -> int (the game uses its own seeded generator)
        self.queue = []                # room events queued by ops: (type, arg)   (op 09 -> type 19)
        self.timer = None              # one-shot room timer set by op 2A: (arg, delay)   -> event 20

    # -- operand helpers -------------------------------------------------------------------
    def obj(self, tid):
        if tid >= 0x8000:              # negative ids: -1 = the current object
            tid = self.cur if tid == 0xFFFF else None
        return self.things.get(tid)

    def _flag_test(self, rec, mask, byte=3):
        if rec is not None and rec[byte] & mask:
            self.c605 = (self.c605 + 1) & 255
        else:
            self.c605 = 0

    # -- opcode semantics ------------------------------------------------------------------
    def op(self, op, a):
        """a = operand bytes.  Returns True when implemented."""
        w = (a[0] << 8 | a[1]) if len(a) >= 2 else 0
        r = self.obj(w) if len(a) >= 2 else None
        cur = self.things.get(self.cur)
        if op == 1 and r is not None:               # place: clear 0x80; a thing that belongs to another room is only flagged 0x08 (pending)
            had = r[3] & 0x80
            r[3] &= 0x7F
            if had and r[10] != (self.room & 255):
                r[3] |= 0x08
        elif op == 26 and r is not None:            # hide
            r[3] |= 0x80
        elif op in (15, 16):                        # test flag bit0
            self._flag_test(r, 1)
        elif op == 17 and r is not None:            # set bit0
            r[3] |= 1
        elif op == 24 and r is not None:            # toggle bit0
            r[3] ^= 1
        elif op == 18 and r is not None:            # clear bit0
            r[3] &= ~1 & 255
        elif op == 19:                              # test bit1
            self._flag_test(r, 2)
        elif op == 20 and r is not None:            # set bit1
            r[3] |= 2
        elif op == 25 and r is not None:            # toggle bit1
            r[3] ^= 2
        elif op == 21 and r is not None:            # clear bit1
            r[3] &= ~2 & 255
        elif op == 54 and r is not None:            # set flags2 bit 2
            r[15] |= 4
        elif op == 55 and r is not None:            # clear flags2 bit 2
            r[15] &= ~4 & 255
        elif op == 68 and r is not None:            # set flags2 bit 6
            r[15] |= 0x40
        elif op == 67 and r is not None:            # clear flags2 bit 6
            r[15] &= ~0x40 & 255
        elif op == 30:                              # test bit0 of the current object
            self._flag_test(cur, 1)
        elif op == 31 and cur is not None:
            cur[3] &= ~1 & 255
        elif op == 32 and cur is not None:
            cur[3] |= 1
        elif op == 33 and cur is not None:
            cur[3] ^= 1
        elif op == 29:                              # disarm the running handler
            self.disarm = True
        elif op == 5:                               # score += n
            self.score = (self.score + a[0]) & 0xFFFFFFFF
        elif op == 38:                              # var[i] = k
            self.vars[a[0]] = a[1]
        elif op == 39:                              # var[i] += k
            self.vars[a[0]] = (self.vars[a[0]] + a[1]) & 255
        elif op == 40:                              # test var[i] <mode> k
            v, mode, k = self.vars[a[0]], a[1], a[2]
            ok = (v > k, v < k, v == k)[mode] if mode < 3 else v != k
            if ok:
                self.c605 = (self.c605 + 1) & 255
            else:
                self.c605 = 0
        elif op == 47:                              # test: the thing exists (id resolves and is not 0)
            self.c605 = (self.c605 + 1) & 255 if r is not None and (w == 0xFFFF and self.cur or w) else 0
        elif op in (28, 72):                        # say(string) / describe: queues a message (ds:628, the 'message shown' flag, is user-interface state)
            self.messages.append((op, w))
        elif op == 79:                              # reg = random(a, b)
            self.reg = self.rand(a[0], a[1]) if self.rand else a[0]
        elif op == 80:                              # test reg == a
            self.c605 = (self.c605 + 1) & 255 if self.reg == a[0] else 0
        elif op == 9:                               # queue room event 19 with a byte argument
            self.queue.append((19, a[0]))
        elif op == 42:                              # one-shot room timer: after `delay` timer units fire event 20 with arg
            self.timer = (a[0], a[1])
        elif op == 81:                              # reg = var[a]
            self.reg = self.vars[a[0]]
        elif op == 43:                              # test the room of a thing
            room = self.room if a[2] == 0xFE else a[2]
            if r is not None and r[10] == room:
                self.c605 = (self.c605 + 1) & 255
            else:
                self.c605 = 0
        else:
            self.unsupported.append(op)
            return False
        return True


# ---------------------------------------------------------------------------------------------- running handler lists
def run_ops(vm, ops):
    """Execute a parsed op list (acl.parse_ops structure)."""
    for o in ops:
        if isinstance(o, list):                       # IF block
            head, then, *els = o
            op, hdr = head
            if op == 14:
                cond = vm.c605 != 0
            elif op == 48:
                cond = vm.c605 == 0
            elif op == 58:
                cond = vm.c605 == hdr[0]
            else:                                     # 59
                cond = vm.c605 != hdr[0]
            vm.c605 = 0
            if cond:
                run_ops(vm, then[1])
            elif els:
                run_ops(vm, els[0][1])
            continue
        if o[0] in (0x16, 0x17):
            continue
        vm.op(o[0], o[1])


def fire(vm, entries, event, armed=None):
    """Run every armed handler of `event`; `entries` = acl.parse_list result; armed = list of bools (modified)."""
    armed = armed if armed is not None else [True] * len(entries)
    for i, h in enumerate(entries):
        if h['type'] != event or not armed[i]:
            continue
        vm.disarm = False
        vm.c605 = 0
        run_ops(vm, h['ops'])
        if not h['keep'] or vm.disarm:
            armed[i] = False
    return armed


def cond_match(h, typ, arg):
    """Condition test of a handler entry for a room event (CS:76FC table): types 6, 14, 28 always match; 15, 17, 19, 20, 24 compare the
    operand byte with the low byte of the event argument."""
    if typ in (15, 17, 19, 20, 24):
        return h['cond'][0] == (arg & 0xFF)
    return True


def fire_event(vm, entries, typ, arg=0, armed=None):
    """Deliver one room event (and everything the handlers queue, in order) to a room's handler list, like 1000:7634."""
    armed = armed if armed is not None else [True] * len(entries)
    pending = [(typ, arg)]
    while pending:
        t, a = pending.pop(0)
        for i, h in enumerate(entries):
            if h['type'] != t or not armed[i] or not cond_match(h, t, a):
                continue
            vm.disarm = False
            vm.c605 = 0
            vm.queue = []
            run_ops(vm, h['ops'])
            pending.extend(vm.queue)
            if not h['keep'] or vm.disarm:
                armed[i] = False
    return armed
