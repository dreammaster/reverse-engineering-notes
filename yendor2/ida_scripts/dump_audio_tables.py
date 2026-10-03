"""
LookupMusicTrackBlockOffset / LookupSoundEffectBlockOffset (yendor2.asm:42528 / :42941) index tables of 32-bit WORLD.DAT offsets
plus a parallel table of 16-bit lengths; PrepareMusicDataRead reads a 0x9BD-byte driver data block whose offset is the dword at
DS:0xCE23. Dumps them (Chapter 2: music offsets DS:0xCE77 x 21, lengths 0xCECB; effects 0xCEF5 x 81, lengths 0xD039).

    .\run_ida_script.ps1 dump_audio_tables.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
MUSIC_OFFSETS, MUSIC_LENGTHS, MUSIC_COUNT = 0xCE77, 0xCECB, 21
EFFECT_OFFSETS, EFFECT_LENGTHS, EFFECT_COUNT = 0xCEF5, 0xD039, 81
DRIVER = 0xCE23


def dword(addr):
    return ida_bytes.get_word(DS_BASE + addr) | (ida_bytes.get_word(DS_BASE + addr + 2) << 16)


lines = ["driver data offset %d size 2493" % dword(DRIVER)]
for name, offsets, lengths, count in (("music", MUSIC_OFFSETS, MUSIC_LENGTHS, MUSIC_COUNT), ("effect", EFFECT_OFFSETS, EFFECT_LENGTHS, EFFECT_COUNT)):
    for i in range(count):
        lines.append("%s %d offset %d length %d" % (name, i + 1, dword(offsets + 4 * i), ida_bytes.get_word(DS_BASE + lengths + 2 * i)))
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor2\ida_scripts\audio_tables.txt", "w", encoding="utf-8").write(text)
print(text[:400])
