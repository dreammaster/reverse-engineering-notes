"""
LookupMusicTrackBlockOffset / LookupSoundEffectBlockOffset (yendor3.asm:42528 / :42941) index tables of 32-bit WORLD.DAT offsets
plus a parallel table of 16-bit lengths; PrepareMusicDataRead reads a 0x9BD-byte driver data block whose offset is the dword at
DS:0xCE23. Dumps them (Chapter 3: DS:0xB217 x 24, 0xB277; 0xB2A7 x 141, 0xB4DB; driver 0xB1BB).

    .\run_ida_script.ps1 dump_audio_tables.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
MUSIC_OFFSETS, MUSIC_LENGTHS, MUSIC_COUNT = 0xB217, 0xB277, 24
EFFECT_OFFSETS, EFFECT_LENGTHS, EFFECT_COUNT = 0xB2A7, 0xB4DB, 141
DRIVER = 0xB1BB


def dword(addr):
    return ida_bytes.get_word(DS_BASE + addr) | (ida_bytes.get_word(DS_BASE + addr + 2) << 16)


lines = ["driver data offset %d size 2493" % dword(DRIVER)]
for name, offsets, lengths, count in (("music", MUSIC_OFFSETS, MUSIC_LENGTHS, MUSIC_COUNT), ("effect", EFFECT_OFFSETS, EFFECT_LENGTHS, EFFECT_COUNT)):
    for i in range(count):
        lines.append("%s %d offset %d length %d" % (name, i + 1, dword(offsets + 4 * i), ida_bytes.get_word(DS_BASE + lengths + 2 * i)))
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor3\ida_scripts\audio_tables.txt", "w", encoding="utf-8").write(text)
print(text[:400])
