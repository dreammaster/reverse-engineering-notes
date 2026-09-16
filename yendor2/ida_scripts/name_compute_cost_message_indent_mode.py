"""
Names sub_1CB37, called from sub_1A5F6 and ShowHealingCostPrompt --
both known DrawIndentedTextColumn callers/preparers.

Selects one of 3 threshold-tier tables based on word_3197C (2/6/4/
other), each giving 4 ascending threshold values (ax/dx/di/si) and a
fontOffset (2/6/4/0). Compares the record at word_3197E's own
`[+0x6E]` field (a cost/quantity value) against the selected
thresholds, setting exactly one of word_328C4's indent-mode bits
(0x2/0x4/0x8/0x10, or the fallback 0x20) -- the same bits
DrawIndentedTextColumn dispatches on to choose how many leading lines
of a wrapped cost message get zero indent. Sets `fontOffset` to match.
Reads as: pick the DrawIndentedTextColumn wrapping style appropriate
for how many digits a cost/quantity value has, before drawing a
formatted "IT WILL COST <n> ..." style message. -> ComputeCostMessageIndentMode

Run via:
    .\run_ida_script.ps1 name_compute_cost_message_indent_mode.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1CB37
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ComputeCostMessageIndentMode", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ComputeCostMessageIndentMode': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Picks a DrawIndentedTextColumn wrapping mode (word_328C4 bits "
    "0x2/0x4/0x8/0x10/0x20 + fontOffset) based on which of 4 "
    "ascending thresholds the record at word_3197E's [+0x6E] field "
    "falls into, with the threshold table itself selected by "
    "word_3197C. Called from sub_1A5F6 and ShowHealingCostPrompt "
    "before drawing a wrapped cost message.",
    False,
)

ea = 0x1191E
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ClearVideoBackBufferLowerRegion", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ClearVideoBackBufferLowerRegion': {'ok' if ok else 'FAILED'}")
ida_bytes.set_cmt(
    ea,
    "Zeroes 0x44C0 words (0x8980 bytes) of _videoBufferSeg starting "
    "at offset 0x6900 -- clears the lower portion of the off-screen "
    "back buffer (below the status/text area). Called once from "
    "ShowIntroPicture.",
    False,
)
