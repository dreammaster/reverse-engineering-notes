"""
Names two generic string-drawing helpers found investigating ranked
candidates:

sub_23B76 (0x23B76, called from ShowIntroPicture, DrawCharacterProtectionsList,
DrawAfflictionsList): draws cx consecutive null-terminated strings from
bx, stacked vertically (each row +6 y, x reset per row) -- a simple
"draw a column of labels" utility, used this session to draw the 9
PROTECTIONS/AFFLICTIONS name lists. -> DrawStringColumn

sub_23AF2 (0x23AF2, called 3 times from ShowCharacterSkills's setup):
writes a null-terminated string character by character, using
word_2E414 for every character except the one where an internal
countdown (seeded by the caller's cx) reaches 1, which uses
word_2E412 instead -- i.e. draws a string with exactly one character
(the Nth, N = initial cx) in a different/highlight color. Likely a
hotkey-letter highlight for a tab/menu label, given its 3 call sites
sit in ShowCharacterSkills's panel-switching setup (the same screen
DrawCharacterStatSheet/DrawCharacterProtectionsList/DrawAfflictionsList/
DrawAbilityReadinessList all belong to). -> WriteStringWithHighlightedChar

Run via:
    .\run_ida_script.ps1 name_string_draw_helpers.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x23B76, "DrawStringColumn"),
    (0x23AF2, "WriteStringWithHighlightedChar"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x23B76,
    "Draws cx consecutive null-terminated strings from bx, stacked "
    "vertically (each row +6 y). Used to draw the 9 PROTECTIONS/"
    "AFFLICTIONS name columns, among others.",
    False,
)
ida_bytes.set_cmt(
    0x23AF2,
    "Writes a string char-by-char, coloring every character "
    "word_2E414 except the Nth (N = initial cx), which uses "
    "word_2E412 -- a single-highlighted-character string draw, "
    "plausibly for a hotkey letter in a tab/menu label. Called 3 times "
    "from ShowCharacterSkills's setup.",
    False,
)
