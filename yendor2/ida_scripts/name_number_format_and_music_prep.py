"""
Names three small unrelated helpers found this round.

sub_2801A, called once from the already-named
UpdateAmbientMusicForRegion: configures a resource-read descriptor at
[bx+...] -- writes the caller's ax into [bx+4], copies a fixed pair of
words from table 0xCDFB into [bx+0xA]/[bx+0xC], and writes global
_blockSize5 into [bx+6] -- the same shape as this codebase's other
resource-stub helpers (sub_27DE5/sub_27DC6/sub_27E3A), but specific to
ambient-music region data. -> PrepareAmbientMusicBlockRead

sub_2570C, called from ShowItemAbilityEffectInfo and others: strips
commas and spaces from an in-place string (copies byte-by-byte,
skipping ',' and ' ', until the terminating NUL). The natural
counterpart to FormatNumber's thousands-separator insertion.
-> StripCommasAndSpaces

sub_28138, called repeatedly from unnamed sub_28034: formats a number
into the shared 0xAFA8 scratch buffer via FormatNumber, then
immediately strips its separators back out via StripCommasAndSpaces,
returning the buffer pointer in ax -- i.e. produces a compact
(separator-free) numeric string. -> FormatNumberCompact

Run via:
    .\run_ida_script.ps1 name_number_format_and_music_prep.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x2801A: "PrepareAmbientMusicBlockRead",
    0x2570C: "StripCommasAndSpaces",
    0x28138: "FormatNumberCompact",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2801A,
    "Configures a resource-read descriptor: [bx+4]=ax (caller value), "
    "[bx+0xA]/[bx+0xC]=fixed pair from table 0xCDFB, [bx+6]="
    "_blockSize5. Same shape as sub_27DE5/sub_27DC6/sub_27E3A. Called "
    "from UpdateAmbientMusicForRegion.",
    False,
)
ida_bytes.set_cmt(
    0x2570C,
    "Strips ',' and ' ' from an in-place NUL-terminated string -- the "
    "counterpart to FormatNumber's thousands-separator insertion. "
    "Called from ShowItemAbilityEffectInfo and others.",
    False,
)
ida_bytes.set_cmt(
    0x28138,
    "FormatNumber into the shared 0xAFA8 buffer, then "
    "StripCommasAndSpaces on it -- produces a compact, separator-free "
    "numeric string, returned in ax. Called from unnamed sub_28034.",
    False,
)
