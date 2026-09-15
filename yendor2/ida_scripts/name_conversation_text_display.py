"""
Traced RunConversation's 4 topic-display sub-functions (previously
flagged "plausibly different response categories, not confirmed
which"). All 4 share the same mechanism -- a paginated conversation-
text display: draws a portrait icon (g_pictureDir entry 7), then
word-wraps and pages the NPC's response text in a 2-column layout
(13 chars/column via sub_28A76), waiting for a keypress between pages
until the text is exhausted or ESC is pressed.

They differ in: which "prep" function loads the text (sub_2B71D for
sub_2B78D, sub_2B755 for sub_2B866, and presumably 2 more for the
other pair -- not all checked), and the screen position/font color
used (e.g. sub_2B78D at x=0x1A,y=0x21 color 0x39 vs sub_2B866 at
x=0x39,y=0x2B color 0x46). All 4 read the SAME record field
(word_2E548's [+4]), so the distinction isn't which topic data they
read -- it's genuinely unclear whether these represent different
NPCs' simultaneous dialogue, different visual/emotional framings, or
something else. Named by their RunConversation dispatch bit (matching
the UseItemType_400 naming convention for similarly-ambiguous cases)
rather than guessing a specific topic category.

Run via:
    .\run_ida_script.ps1 name_conversation_text_display.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x2B78D: "ShowConversationText_4000",
    0x2B866: "ShowConversationText_2000",
    0x2B8D7: "ShowConversationText_1000",
    0x2B948: "ShowConversationText_800",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

for ea in RENAMES:
    ida_bytes.set_cmt(
        ea,
        "One of RunConversation's 4 topic-display branches (selected "
        "by word_2E548's [+2] flag bits). Draws a portrait icon "
        "(g_pictureDir entry 7) then paginates the NPC's response "
        "text in a 2-column layout, waiting for a keypress between "
        "pages. All 4 read the same text field ([+4]) but use "
        "different prep functions and screen position/color -- exact "
        "distinction between them not confirmed.",
        False,
    )
