"""
Refreshes RunConversation's own comment to reference the now-named
ShowConversationText_* functions instead of stale sub_ names.

Run via:
    .\run_ida_script.ps1 update_runconversation_comment.py
"""
import idc
import ida_bytes

ea = idc.get_name_ea_simple("RunConversation")
print(f"RunConversation @ {ea:#x}")

new_cmt = (
    "NPC conversation display: draws the dialog panel (g_pictureDir "
    "entry 1) then dispatches to one of 4 topic-display functions "
    "(ShowConversationText_4000/_2000/_1000/_800) based on flag bits "
    "in the record at word_2E548 (+2). Each is a paginated text "
    "display (portrait icon + 2-column word-wrap) -- all 4 read the "
    "same text field, differing only in prep function and screen "
    "position/color, so which topic category each represents isn't "
    "confirmed. word_2E548's record shares a status-flags field "
    "(+0x1C) with the party-member records RunTitleScreen touches."
)
ida_bytes.set_cmt(ea, new_cmt, False)
print("comment updated")
