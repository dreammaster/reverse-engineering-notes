"""
Read-only: dumps g_trapEffectDefs, the table of 12-byte trap/status effect
definitions (PrepareTrapEffectSlots: base DS:0x905B, id * 12), as rows of six
u16 words, for src23/effect.c and docs23/file-formats.md.

Fields: +0 sound id, +2 icon picture id, +4/+6 magnitude min/max (or fixed),
+8 cost flags (0x1/0x4/0x2 gold/ore/ore, 0x8 MP, 0x10 HP, 0x20 HP+MP; high
bits 0xFF80 = status conditions inflicted), +0xA mode flags.

Note InitGlobals overwrites word 0 of entry 0 and entry 22 (`[905B]` and
`[905B+0x108]`) with _val20 at startup.

    .\run_ida_script.ps1 dump_trap_effects.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
BASE = 0x905B
COUNT = 100

print("TRAP EFFECTS @DS:%#x x%d" % (BASE, COUNT))
for i in range(COUNT):
    words = [ida_bytes.get_wide_word(DS_BASE + BASE + i * 12 + 2 * k) for k in range(6)]
    print("  %2d: %s" % (i, " ".join("%04x" % w for w in words)))
