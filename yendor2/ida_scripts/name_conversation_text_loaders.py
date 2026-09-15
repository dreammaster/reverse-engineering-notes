"""
Follow-up: checked the 4 "prep" functions ShowConversationText_4000/
_2000/_1000/_800 call before displaying text, and they resolve the
ambiguity left open last round. Each calls a DIFFERENT resource-stub
function (sub_27B0D/sub_27CC9/sub_27D20/sub_27D55 -- part of the
~27-function WORLD.DAT resource-stub cluster documented much earlier
this session) to read from WORLD.DAT (FileEntry bx=0x9043,
errorCode=0xD) into the shared text buffer (0xAFA8). So the 4
ShowConversationText_* branches DO read 4 genuinely distinct data
sources -- confirms the original "plausibly different response
categories" guess was right, even though which specific category
(Name/Job/Bye/Rumor-style) each represents still isn't identified
(the resource stubs themselves just carry fixed catalog offsets, not
semantic labels).

Named the 4 loaders in parallel with their matching display function.

Run via:
    .\run_ida_script.ps1 name_conversation_text_loaders.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x2B71D: "LoadConversationText_4000",
    0x2B739: "LoadConversationText_1000",
    0x2B755: "LoadConversationText_2000",
    0x2B771: "LoadConversationText_800",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

for ea in RENAMES:
    ida_bytes.set_cmt(
        ea,
        "Reads this topic's text from WORLD.DAT (FileEntry bx=0x9043, "
        "errorCode=0xD) into the shared text buffer at 0xAFA8, via a "
        "resource-stub helper carrying this topic's fixed catalog "
        "offset. Confirms the 4 ShowConversationText_* branches read "
        "genuinely distinct data, even though the specific topic "
        "category isn't identified.",
        False,
    )
