"""
Event script disassembler.  python mm2_events.py MAP [MAP ...]   (map 0-59; indoor files for maps 0-4 and 17+, outdoor 5-16)
Prints the trigger table, every script with named opcodes, and the message texts (chars & 7Fh, 40h = newline).
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mm2_data import Game, split_scripts   # noqa: E402

NAMES = {1: "msg", 2: "msg_row13", 3: "msg_frame", 4: "title", 5: "msg_box", 6: "msg_framed", 7: "wait_key", 8: "wait_key_music",
         9: "ask_yn", 10: "ask_yn2", 11: "show_monster", 12: "teleport", 13: "sound", 14: "location", 15: "end",
         16: "skip_if", 17: "skip_if_not", 18: "fight", 19: "fight2", 20: "clear_trigger", 21: "char_check", 22: "has_item",
         23: "var_cond", 24: "char_modify", 25: "give_item", 26: "set_var", 27: "cond_gt", 28: "cond_rand", 29: "wait", 30: "wait_key_abort",
         31: "modify_char", 32: "modify_party", 33: "set_cell", 34: "time_check", 35: "date_check", 36: "pay_gold", 37: "pay_gems",
         38: "choose_char", 39: "choose_char2", 40: "remove_item", 41: "stop", 42: "place_treasure", 43: "skip_if_night",
         44: "add_value", 45: "check_class_race_align", 46: "teach", 47: "read_string", 48: "compare_string", 49: "award_exp",
         50: "party_skill"}
MSG_OPS = {1, 2, 3, 4, 5, 6}


def messages(raw):
    out, cur = [], []
    for b in raw:
        if b == 0xFF:
            out.append("".join(cur))
            cur = []
        else:
            c = b & 0x7F
            cur.append("\n" if c == 0x40 else chr(c))
    return out


def dump(g, m):
    kind = "O" if 5 <= m <= 16 else "I"
    chunk = g.events(kind, m)
    if not chunk:
        print("map %d: no events" % m)
        return
    trig, body, msgs = g.parse_events(chunk)
    texts = messages(msgs)
    print("== map %d (%s) ==" % (m, kind))
    for cell, script, mask in trig:
        dirs = "".join(c for c, bit in (("N", 0x80), ("E", 0x40), ("S", 0x20), ("W", 0x10)) if mask & bit)
        print("  trigger x=%d y=%d script %d facing %s" % (cell & 15, cell >> 4, script, dirs or "%02X" % mask))
    for i, sc in enumerate(split_scripts(body)):
        print("  script %d:" % i)
        for op, a in sc:
            line = "    %-14s %s" % (NAMES.get(op, "op%d" % op), a.hex(" "))
            if op in MSG_OPS and a and a[0] - 1 < len(texts) and a[0] >= 1:
                line += "   ; " + texts[a[0] - 1].replace("\n", " / ")[:90]
            print(line)


if __name__ == "__main__":
    g = Game()
    for m in (int(x) for x in sys.argv[1:]) or range(60):
        dump(g, m)
