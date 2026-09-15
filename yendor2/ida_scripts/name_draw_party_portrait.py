"""
Names sub_19133 and sub_2607F -- the party-member portrait renderer
called throughout RefreshPartyPortraits/HandlePortraitClick.

sub_19133 (-> ShowPartyPortraitForSlot): resolves a slot's record id
([si]) to a record pointer (sub_25B14) then calls the actual portrait
drawer.

sub_2607F (-> DrawPartyMemberPortrait): draws one party member's
portrait panel at the caller-set position (word_328BC/word_328C0):
the character's own icon ([+0x14] via DrawPicture), a status bar
(sub_267A7, fields selected by a status flag at [+0x15C] bit 0x1000),
a condition icon (bit 0x20, positions from table 0x6166) when a
different condition ([+0x10]) applies, and further icon draws for
fields at [+0x17C]/[+0x180] (sub_2681B, not traced). A 150-line
function; only the overall shape and the fields already documented
elsewhere (party record `+0x14` portrait icon, `+0x10` a per-tier
flag) are confirmed, the rest of its icon-selection logic isn't
individually traced.

Run via:
    .\run_ida_script.ps1 name_draw_party_portrait.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x19133: "ShowPartyPortraitForSlot",
    0x2607F: "DrawPartyMemberPortrait",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x19133,
    "Resolves a party slot's record id to a pointer (sub_25B14) then "
    "draws its portrait (DrawPartyMemberPortrait). Called from "
    "RefreshPartyPortraits/HandlePortraitClick.",
    False,
)
ida_bytes.set_cmt(
    0x2607F,
    "Draws one party member's portrait panel at word_328BC/word_328C0: "
    "character icon ([+0x14]), a status bar (sub_267A7), a condition "
    "icon ([+0x15C]/[+0x10]), and further icon draws (sub_2681B, not "
    "traced). Called via ShowPartyPortraitForSlot.",
    False,
)
