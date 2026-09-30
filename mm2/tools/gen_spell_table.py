"""Generate docs/spells.md: SPELLS.DAT raw entries joined with the manual's spell list
(pdftotext -layout Manual.pdf manual.txt must exist; path given as argv[1])."""
import os
import re
import sys

txt = open(sys.argv[1], encoding="utf-8", errors="replace").read()
cur, spells = None, []
for l in txt.split("\n"):
    m = re.match(r"\s*(Cleric|Sorcerer) Spells Level (\d)", l)
    if m:
        cur = (m.group(1), int(m.group(2)))
        continue
    m = re.match(r"\s*\d*[.,]?\s*name:\s*(.*)", l, re.I)
    if m and cur and not l.strip().lower().startswith("name: generally"):
        spells.append({"div": cur[0], "level": cur[1], "name": m.group(1).strip()})
        continue
    if spells:
        for k in ("COST", "TYPE", "OBJECT"):
            m = re.match(r"\s*" + k + r":\s*(.*)", l)
            if m and k.lower() not in spells[-1]:
                spells[-1][k.lower()] = m.group(1).strip()
d = open(os.path.join(r"D:\GOG Games\Might and Magic 2", "SPELLS.DAT"), "rb").read()
order = [s for s in spells if s["div"] == "Sorcerer"] + [s for s in spells if s["div"] == "Cleric"]
out = ["| Idx | Division | Lvl | Name | Manual cost | Manual type | b0 | b1 |", "|--:|---|--:|---|---|---|---|---|"]
for i, s in enumerate(order):
    out.append("| %d | %s | %d | %s | %s | %s | %02X | %02X |" % (
        i, s["div"], s["level"], s["name"], s.get("cost", ""), s.get("type", ""), d[2 * i], d[2 * i + 1]))
print("\n".join(out))
