"""
Names sub_23BAE and resolves RunTitleScreen's 'C' option: iterates a
linked list at word_328D4 (plausibly the party members, matching the
"up to 4 party-member records" pattern RunTitleScreen's 'E' handler
already established) via [si+0x10], running a pipeline of per-member
display steps (sub_243D3, sub_24A5B, sub_24BF2, sub_245AE, sub_2498B,
sub_25103 -- 'Q' aborts the loop at each stage) -- viewing each party
member's details in turn. sub_245AE (989 bytes, DrawListEntryLabel's
only caller, references 8 generic globals _val1-_val8 -- plausibly
skill or stat categories) is the biggest single step but too large to
trace this round.

-> ShowPartyMembers

Run via:
    .\run_ida_script.ps1 name_show_party.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x23BAE
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowPartyMembers", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowPartyMembers': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Iterates the party-member list (word_328D4, via [si+0x10]), "
    "running a pipeline of per-member display steps (sub_243D3, "
    "sub_24A5B, sub_24BF2, sub_245AE, sub_2498B, sub_25103) -- 'Q' "
    "aborts at any stage. RunTitleScreen's only caller (its 'C' "
    "option) -- resolves 'C' as viewing the party's characters.",
    False,
)

ida_bytes.set_cmt(
    0x1D2A6,
    "Main title screen: draws g_pictureDir entry 2 (combat scene) "
    "full-screen + mouse cursor, then dispatches 5 menu options -- "
    "selectable by keyboard (C/A/E/R/I) or mouse click (numeric "
    "codes 1-5 from sub_1D118, funneled into the same handler labels). "
    "C: ShowPartyMembers (view party characters). "
    "A: sub_25862+sub_2BD1A, redraw. E: sets a flag on up to 4 "
    "party-member records then RETURNS from the function entirely -- "
    "this is what actually leaves the title screen and proceeds into "
    "the game (plausibly 'Enter'). R: sub_25862+ShowIntroPicture, "
    "redraw (plausibly 'About'/replay intro, or a registration-info "
    "screen given this shareware build's nag string). "
    "I: RunCharacterCreation (plausibly 'Import', given this is "
    "Chapter 2 of a series). Called from `start` and from "
    "ConfirmNewGame after confirming a new game.",
    False,
)
