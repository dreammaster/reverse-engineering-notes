"""
Names sub_2B656: draws the dialog-panel background (g_pictureDir entry
1, same panel RunGameDialog uses) then dispatches to one of 4 sibling
functions based on flag bits in a record pointed to by word_2E548
(bx+2 bits 0x4000/0x2000/0x1000/0x800) -- each sibling draws g_pictureDir
entry 7 (32x32 icon) plus word-wrapped text (26-char width) from a
per-topic buffer. word_2E548's record also has a status-flags field at
+0x1C matching the one RunTitleScreen's 'E' handler sets on party-member
records -- so this operates on a character/NPC record. Starts with a
ShowConfirmPrompt(msg=0xB) gate. Matches the manual's "talk to an NPC"
feature and the "Problem retreiving conversation data" error string.

-> RunConversation

The 4 sibling functions (sub_2B78D/2B866/2B8D7/2B948) are plausibly
different conversation-topic/response categories but aren't
individually distinguishable from static analysis alone (same caution
as the earlier resource-stub cluster) -- documented, not individually
renamed.

Run via:
    .\run_ida_script.ps1 name_conversation.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2B656
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunConversation", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunConversation': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "NPC conversation display: draws the dialog panel (g_pictureDir "
    "entry 1) then dispatches to one of 4 topic-display functions based "
    "on flag bits in the record at word_2E548 (+2, bits 0x4000/0x2000/"
    "0x1000/0x800). Each draws a small icon (entry 7) plus word-wrapped "
    "text. word_2E548's record shares a status-flags field (+0x1C) with "
    "the party-member records RunTitleScreen touches. Not individually "
    "distinguishing the 4 topic-type sub-functions (sub_2B78D/2B866/"
    "2B8D7/2B948) -- plausibly different response categories, not "
    "confirmed which.",
    False,
)
