#!/usr/bin/env python3
"""Decode Legacy of the Ancients' music -- the GW-BASIC `PLAY` MML strings
compiled into `MENU.EXE` (title + menu theme) and `CELDRV.EXE` (endgame,
same tune).  There is no music *file* -- the tunes are string constants
handed to `basPlayMusic` (`rtm_CE`, leglib seg003:0x1edba), which drives
the PC-speaker one voice at a time.

    python decoders/music_mml.py [C:\\games\\lota\\MENU.EXE]

The 6 phrases in MENU.EXE (CELDRV.EXE has phrases 1-5):

  title theme, played looped by showTitleScreen -> playMusicTick:
    1  l4cl8g>mll4c.mncl8c<bab>l2c.<l4g.al8b>l4c<l8gmll4e.mnel8dl4el8fl1c.
    2  >l4c<l8amll4g.mngl8fl4gl8amll2emnl8edl2c.l4c.el8fl4g.fl8al1g.
    3  >l4c<l8amll4g.mngl8fl4gl8al2mlemnl8edl2c.l4c.gl8fl4el8dl4el8fl1c.
    (an earlier near-variant of #1 also lives at the aDL4cl8g label)
  menu theme, played by menuStartup (ds:210A / ds:210E):
    5  t120 l4cl8efl4gl8efl4dgcl8efl4al8efl4gl8efl1d
    6  l4cl8efl4gl8efl4dgcl8efl4al8gel4fl8edl1c t180

MML dialect (GW-BASIC PLAY):
    o0..o6   set octave (default o4)         >  <   octave up / down
    l1..l64  default note length (1=whole)   .      dot (1.5x) -- repeatable
    t32..255 tempo, quarter-notes/min (def 120)
    a-g      note (optionally + / # / - accidental, then optional length)
    p / r    rest (+ length)                 n0..n84  note by number
    mn ml ms mf mb  articulation / fg-bg (mn = 7/8 of the beat sounds,
                    ml = full, ms = 3/4; mf/mb ignored on one voice)

The leading `0` / `1` / `80` / `81` digits and trailing `,` you see in a
raw dump are DGROUP string-table framing, not MML.
"""
import re
import sys

NOTE_SEMI = {"c": 0, "d": 2, "e": 4, "f": 5, "g": 7, "a": 9, "b": 11}
PHRASES = [
    "l4cl8g>mll4c.mncl8c<bab>l2c.<l4g.al8b>l4c<l8gmll4e.mnel8dl4el8fl1c.",
    ">l4c<l8amll4g.mngl8fl4gl8amll2emnl8edl2c.l4c.el8fl4g.fl8al1g.",
    ">l4c<l8amll4g.mngl8fl4gl8al2mlemnl8edl2c.l4c.gl8fl4el8dl4el8fl1c.",
    "t120l4cl8efl4gl8efl4dgcl8efl4al8efl4gl8efl1d",
    "l4cl8efl4gl8efl4dgcl8efl4al8gel4fl8edl1ct180",
]
TOK = re.compile(r"(o\d|[<>]|l\d+|t\d+|m[nlsfb]|n\d+|[a-gpr][+#-]?\d*\.*)",
                 re.IGNORECASE)


def parse(mml, octave=4, length=4, tempo=120):
    """-> list of (name, freq_hz|None, ms).  freq None = rest."""
    out = []
    for tok in TOK.findall(mml.replace(" ", "").lower()):
        if tok in ("<", ">"):
            octave = max(0, min(6, octave + (1 if tok == ">" else -1)))
        elif tok[0] == "o":
            octave = int(tok[1:])
        elif tok[0] == "l" and tok[1:].isdigit():
            length = int(tok[1:])
        elif tok[0] == "t":
            tempo = int(tok[1:])
        elif tok[0] == "m":
            pass  # articulation -- affects gate time, not pitch
        elif tok[0] == "n":
            num = int(tok[1:])
            ms = 60000 / tempo * 4 / length
            if num == 0:
                out.append(("rest", None, ms))
            else:
                freq = 440.0 * 2 ** ((num - 1 - 57) / 12)
                out.append((f"n{num}", freq, ms))
        else:
            m = re.match(r"([a-gpr])([+#-]?)(\d*)(\.*)", tok)
            note, acc, ln, dots = m.groups()
            dur = int(ln) if ln else length
            ms = 60000 / tempo * 4 / dur
            for _ in dots:
                ms *= 1.5
            if note in ("p", "r"):
                out.append(("rest", None, ms))
            else:
                semi = NOTE_SEMI[note] + (1 if acc in "+#" else -1 if acc == "-" else 0)
                midi = (octave + 1) * 12 + semi        # o4 c = MIDI 60
                freq = 440.0 * 2 ** ((midi - 69) / 12)
                out.append((f"{note}{acc}{octave}", freq, ms))
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else r"C:\games\lota\MENU.EXE"
    try:
        blob = open(path, "rb").read()
        found = re.findall(rb"[<>.#+\-a-go0-9lltnms]{20,}", blob)
        found = [s.decode("latin1") for s in found
                 if re.search(r"[lt][0-9]", s.decode("latin1"))
                 and sum(s.lower().count(c) for c in b"abcdefg") >= 6]
        print(f"{path}: {len(found)} MML-looking strings\n")
    except OSError:
        found = []

    for i, ph in enumerate(PHRASES, 1):
        print(f"--- phrase {i} ---")
        print(f"  {ph}")
        ev = parse(ph)
        line = " ".join(e[0] if e[1] is None else e[0] for e in ev)
        print(f"  {line}")
        total = sum(e[2] for e in ev)
        print(f"  {len(ev)} events, ~{total/1000:.1f}s\n")


if __name__ == "__main__":
    main()
