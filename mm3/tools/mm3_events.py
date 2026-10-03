#!/usr/bin/env python3
"""Disassemble MM3 maze event scripts (MAZEnn.EVT, see docs/data-files.md).

usage: mm3_events.py FILE.EVT [TEXTnn.MAZ]
Record: len, x, y, facing(4 = any), line, opcode, operands.  Condition/value operands are (mode, value) pairs whose
value width depends on the mode (same encoding as Xeen's Scripts::cmdIf): 4 bytes for modes 16 and 34, 2 bytes for 25
and 35, otherwise 1 byte.
"""
import struct
import sys

OPS = {
    1: ("Display1", "t"), 2: ("DoorTextSml", "t"), 3: ("DoorTextLrg", "t"), 4: ("SignText", "t"),
    5: ("NPC", "5"), 6: ("PlayFX", "1"), 7: ("Teleport", "3"), 8: ("If>=", "P1L"), 9: ("If==", "P1L"), 10: ("If<=", "P1L"),
    11: ("MoveObj", "3"), 12: ("TakeOrGive", "P2"), 14: ("Remove", ""), 15: ("SetChar", "1"), 16: ("Spawn", "4"),
    17: ("DoTownEvent", "1"), 18: ("Exit", ""), 19: ("AlterMap", "4"), 20: ("GiveMulti", "P3"), 21: ("ConfirmWord", "4"),
    22: ("Damage", "3"), 23: ("JumpRnd", "3"), 24: ("AlterEvent", "2"), 25: ("CallEvent", "3"), 26: ("Return", ""),
    27: ("SetVar", "P1"), 28: ("TakeOrGive2", "P2"), 29: ("TakeOrGive3", "P3"), 30: ("CutsceneEnd", ""),
    31: ("Teleport2", "3"), 32: ("WhoWill", "1"), 33: ("TakeOrGive4", "P2"),
}
WIDE4 = {16, 34}
WIDE2 = {25, 35}

MODES = {3: "sex", 4: "race", 5: "class", 8: "hp", 9: "sp", 10: "ac", 11: "levelBonus", 12: "age", 13: "skill", 15: "award",
         16: "exp", 18: "condition", 19: "spell", 20: "gameFlag", 21: "item", 25: "minutes", 34: "gold", 35: "gems",
         37: "might+", 38: "intellect+", 39: "personality+", 40: "endurance+", 41: "speed+", 42: "accuracy+", 43: "luck+",
         44: "yesno", 64: "level", 65: "food", 76: "day", 85: "year"}


def take_pair(b, i):
    mode = b[i]
    i += 1
    w = 4 if mode in WIDE4 else 2 if mode in WIDE2 else 1
    if i + w > len(b):
        raise ValueError("short")
    val = int.from_bytes(b[i:i + w], "little")
    return "%s(%d)=%d" % (MODES.get(mode, "m%d" % mode), mode, val), i + w


def decode(rec):
    x, y, d, line, op = rec[0], rec[1], rec[2], rec[3], rec[4]
    args = rec[5:]
    name, shape = OPS.get(op, ("op%d" % op, "?"))
    text = ""
    ok = True
    try:
        if shape == "?" or shape == "":
            text = args.hex(" ") if args else ""
            ok = shape == "" and not args or shape == "?"
        elif shape == "t":
            text = "text %d" % args[0]
            ok = len(args) == 1
        elif shape[0] == "P":
            n = int(shape[1])
            i, parts = 0, []
            for _ in range(n):
                s, i = take_pair(args, i)
                parts.append(s)
            if shape.endswith("L"):
                parts.append("goto line %d" % args[i])
                i += 1
            text = " ".join(parts)
            ok = i == len(args)
        else:
            text = " ".join(str(a) for a in args)
            ok = len(args) == int(shape)
    except (ValueError, IndexError):
        text, ok = args.hex(" "), False
    return x, y, d, line, name, text, ok


def load_texts(path):
    """TEXTnn.MAZ: NUL-terminated strings; text index n is the n-th string (0-based)."""
    raw = open(path, "rb").read()
    return [t.decode("latin1").replace(chr(10), " / ").replace(chr(13), "") for t in raw.split(bytes([0]))]


def main():
    data = open(sys.argv[1], "rb").read()
    texts = load_texts(sys.argv[2]) if len(sys.argv) > 2 else None
    bad = 0
    i = 0
    while i < len(data):
        n = data[i]
        if n < 5 or i + 1 + n > len(data):
            break
        x, y, d, line, name, text, ok = decode(data[i + 1:i + 1 + n])
        if texts and text.startswith("text "):
            k = int(text.split()[1])
            text += "  " + repr(texts[k][:70]) if k < len(texts) else ""
        if not ok:
            bad += 1
        print("(%2d,%2d) %s  line %-2d %-12s %s%s" % (x, y, "ANY" if d == 4 else "NESW"[d] + "  ", line, name, text, "" if ok else "   ; operand shape mismatch"))
        i += 1 + n
    if bad:
        print("; %d records did not match the assumed operand shape" % bad, file=sys.stderr)


if __name__ == "__main__":
    main()
