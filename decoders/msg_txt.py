#!/usr/bin/env python3
"""Decode `TWNMSG.TXT` / `MUSMSG.TXT` -- Legacy of the Ancients' message
banks (town rumours; museum-exhibit narration).

    python decoders/msg_txt.py [C:\\games\\lota\\MUSMSG.TXT] [--raw]

Format
------
    word[0]                 = byte offset of the first message (also the
                              size of the pointer table).
    pointer table           = word[0]/2 entries of 1-BASED byte offsets;
                              only the leading run that is monotonic and
                              in range is real (the rest is zero pad).
    text                    = messages back to back, each ending `\\r`
                              (0x0D).

Message markup (kept with --raw, otherwise interpreted / stripped):
    `*`  `=`     hard line break
    `^`          paragraph / scroll-and-continue
    `%`          end of message + wait-for-key
    `\\r`         message terminator (also `%`/`@` at the tail)
    `@X`         a scripted action follows (reward hand-out etc.)
    `#X`         a display directive -- X is one byte: `d`/`2`/`(` pacing,
                 `F` full-stop, `x`/`n`/`<` and raw 0x14/0x1E = colour or
                 delay codes
    `]...]`      highlighted span (place / item name)

`STDRVSCR.DAT` is NOT one of these -- despite the `.DAT` name it is
0xAA-filled CGA screen graphics for the Stones-of-Wisdom table, not a
script (STDRV narrates its rules from strings in the EXE).
"""
import re
import struct
import sys


def messages(d):
    first = struct.unpack_from("<H", d, 0)[0]
    ntab = first // 2
    ptrs = list(struct.unpack_from(f"<{ntab}H", d, 0))
    cnt = 1
    while cnt < ntab and ptrs[cnt] >= ptrs[cnt - 1] and 0 < ptrs[cnt] <= len(d) + 1:
        cnt += 1
    out = []
    for i in range(cnt):
        s = ptrs[i] - 1
        e = (ptrs[i + 1] - 1) if i + 1 < cnt else len(d)
        m = d[s:e].rstrip(b"\r\x00").decode("latin1")
        if len(m.strip()) > 2:          # drop the trailing "." pad entry
            out.append(m)
    return out


def clean(m):
    m = re.sub(r"#.", "", m)           # display directives
    m = m.replace("]", "")             # highlight toggles
    m = m.replace("@", "\n[action] ")  # scripted-action marker
    m = m.replace("%", "")             # end+wait
    m = re.sub(r"[*=^]", "\n", m)      # line / paragraph breaks
    return re.sub(r"\n{3,}", "\n\n", m).strip()


def main():
    args = sys.argv[1:]
    raw = "--raw" in args
    args = [a for a in args if not a.startswith("-")]
    path = args[0] if args else r"C:\games\lota\MUSMSG.TXT"
    d = open(path, "rb").read()
    msgs = messages(d)
    print(f"{path}  {len(d)} B   {len(msgs)} messages\n")
    for i, m in enumerate(msgs):
        print(f"--- [{i}] " + "-" * 50)
        print(m if raw else clean(m))
        print()


if __name__ == "__main__":
    main()
