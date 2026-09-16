"""
Names sub_15429, called once from the very start of the already-named
RunCharacterCreation (before any UI) -- a multi-stage intro animation
sequence, abortable at each stage via the newly-named
PollForEscapeKeyOnlyAlt.

1. Decodes a 768-byte simple-obfuscated (each byte -0x3F) graphics/data
   block from 0x442A into 0x4D5C.
2. Plays music track 0x12, sets word_328C8 bit 0x800, draws the mouse
   cursor.
3. Runs a 63-iteration animation loop (two calls to still-unnamed
   sub_160D6 per frame, conditionally sub_152EF when word_328C4 bit
   0x400 is set -- plausibly a "fast/skip" toggle), then clears a
   768-byte buffer at 0x475A.
4. Checks for ESC (abort) after each of several further staged
   sub-animations (loop counts 5, 0x14, 0xA, 0xA, 0x14, driven by
   still-unnamed helpers sub_16180/sub_1618E/sub_1619F/sub_160C3/
   sub_2589A/sub_160B0), each also honoring the same word_328C4
   bit-0x400 skip check.

In short: the character-creation screen's opening animated sequence.
The low-level draw helpers it calls are not traced/named this round.
-> PlayCharacterCreationIntroAnimation

Run via:
    .\run_ida_script.ps1 name_char_creation_intro_anim.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x15429
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PlayCharacterCreationIntroAnimation", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PlayCharacterCreationIntroAnimation': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Character creation's opening animated sequence: decodes a "
    "768-byte block (byte-0x3F), plays music track 0x12, then runs "
    "several staged sub-animations (63/5/20/10/10/20 frame loops via "
    "unnamed helpers), abortable via PollForEscapeKeyOnlyAlt after "
    "each stage. Called once from RunCharacterCreation.",
    False,
)
