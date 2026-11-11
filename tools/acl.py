#!/usr/bin/env python3
"""
ACL scripts: parser/disassembler for the handler lists stored in thing records (tail from +16, count at byte +11) and room records
(count at +0x1F, list at +0x20).  Operand lengths were measured by running every opcode handler of CAD.EXE (table 1000:7819) in the
emulator on scratch data; structured IF opcodes (14, 48, 58, 59) nest.  docs/formats.md 6.7.

Entry:  [len] [type | 0x80 = keep armed] [condition operands (COND_LEN[type])] [ops ...] 0x17 [pad byte if the length would be odd]      len counts the len byte itself and is always even
"""
import struct

# operand bytes per opcode (handler table at CS:7819; ops 0..93); None = structured IF
OPLEN = {0: 2, 1: 2, 2: 0, 3: 2, 4: 2, 5: 1, 6: 1, 7: 2, 8: 2, 9: 1, 10: 1, 11: 2, 12: 2, 13: 1, 14: None, 15: 2, 16: 2, 17: 2, 18: 2,
         19: 2, 20: 2, 21: 2, 22: 0, 23: 0, 24: 2, 25: 2, 26: 2, 27: 3, 28: 2, 29: 0, 30: 0, 31: 0, 32: 0, 33: 0, 34: 2, 35: 2, 36: 6,
         37: 4, 38: 2, 39: 2, 40: 3, 41: 6, 42: 2, 43: 3, 44: 6, 45: 2, 46: 0, 47: 2, 48: None, 49: 2, 50: 2, 51: 1, 52: 2, 53: 2,
         54: 2, 55: 2, 56: 9, 57: 2, 58: None, 59: None, 60: 4, 61: 1, 62: 4, 63: 4, 64: 5, 65: 3, 66: 3, 67: 2, 68: 2, 69: 5, 70: 1,
         71: 0, 72: 2, 73: 4, 74: 2, 75: 2, 76: 1, 77: 2, 78: 2, 79: 2, 80: 1, 81: 1, 82: 1, 83: 1, 84: 8, 85: 2, 86: 2, 87: 1, 88: 1,
         89: 2, 90: 3, 91: 1, 92: 2, 93: 2}
# operand bytes of the event condition test per event type (table CS:76FC)
COND_LEN = {1: 1, 4: 2, 9: 2, 10: 2, 12: 1, 13: 1, 15: 1, 17: 1, 18: 2, 19: 1, 20: 1, 24: 1, 26: 2}
# meaning of the opcodes read so far (handler addresses in CS; confidence: all from the disassembly, none exercised end-to-end yet)
OPNAMES = {1: 'place_thing(id)        clear flag 0x80 of the thing, put it in the room if it belongs there (7903)',
           5: 'add_score(byte)         ds:24C += n (7A1A)',
           14: 'IF [605]               run the block until 0x16, optional ELSE (0x0F len ...) block (7C3B)',
           26: 'hide_thing(id)         set flag 0x80 of the thing (799A)',
           28: 'say(string id)         print a message (86B1)',
           30: 'test flag bit0 of the current object -> [605] (7D3F)',
           32: 'set flag bit0 of the current object (7D61)',
           37: 'change_room(room,dx,dy,z)  [232] = room, [592]/[594] = landing offsets, calls room_enter (7F3C)',
           39: 'var[n] += k            script variables at ds:611+n (813C)',
           40: 'test var[n] (==, >, <) k -> [605] (8148)',
           72: 'look / describe object (8480)',
           48: 'IF NOT [605] block (7C20)', 58: 'IF [605] == k block (7C32)', 59: 'IF [605] != k block (7C29)'}
END = 0x17


def parse_ops(b, p, end):
    """-> (list of (op, operands bytes) , new position) ; stops after opcode 0x17 (or 0x16 inside an IF block when end == 0x16)."""
    ops = []
    while p < len(b):
        op = b[p]
        p += 1
        if op == end:
            ops.append((op, b''))
            return ops, p
        if op not in OPLEN:
            raise ValueError('unknown opcode %d at %d' % (op, p - 1))
        n = OPLEN[op]
        if op in (62, 63, 64) and p < len(b) and b[p] >= 0x80:      # address with bit 15: byte-sized value
            n -= 1
        if n is None:                                   # IF-like block: [extra byte for 58/59] len, ops..., 0x16, [0F len ops 0x16]
            extra = 1 if op in (58, 59) else 0
            hdr = b[p:p + extra + 1]
            p += extra + 1
            body, p = parse_ops(b, p, 0x16)
            blk = [(op, bytes(hdr)), ('then', body)]
            if p < len(b) and b[p] == 0x0F:
                p += 2 if False else 1
                ln = b[p]
                p += 1
                els, p = parse_ops(b, p, 0x16)
                blk.append(('else', els))
            ops.append(blk)
            continue
        ops.append((op, bytes(b[p:p + n])))
        p += n
    return ops, p


def parse_list(b, start, count):
    """-> list of dict(type, keep, cond, ops, size) ; raises ValueError when the structure does not fit."""
    out, p = [], start
    for _ in range(count):
        ln, tb = b[p], b[p + 1]
        typ = tb & 0x7F
        cl = COND_LEN.get(typ, 0)
        cond = bytes(b[p + 2:p + 2 + cl])
        ops, q = parse_ops(b, p + 2 + cl, END)
        if q != p + ln and not (q + 1 == p + ln and ln % 2 == 0):
            raise ValueError('entry at %d: length %d but parsed %d bytes' % (p, ln, q - p))
        out.append(dict(type=typ, keep=bool(tb & 0x80), cond=cond, ops=ops, size=ln))
        p += ln
    return out


def thing_handlers(rec):
    return parse_list(rec, 16, rec[11]) if len(rec) > 16 and rec[11] else []


def room_handlers(rec):
    return parse_list(rec, 0x20, rec[0x1F]) if len(rec) > 0x20 and rec[0x1F] else []
