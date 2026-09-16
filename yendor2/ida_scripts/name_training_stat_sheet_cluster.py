"""
Names a 3-function cluster shared between UseItemType_400's paid-
service branch and UseTrainingItem: an interactive "select a party
member, show a live stat sheet, track remaining uses" loop.

sub_1978F (called throughout sub_197B9, and elsewhere): draws a
number (ax) in one of two colors depending on whether it exceeds a
threshold (bx) -- normal color (word_2E412) if ax<=bx, an alternate
color (word_2E414) if ax>bx. A generic "value, colored if over a cap"
cell renderer. -> DrawStatValueWithCapColor

sub_197B9 (called from sub_193BE and sub_19553): redraws a full
stat-sheet grid for the current party member (word_328D4) using
DrawStatValueWithCapColor per cell -- 6 rows of base/derived pairs at
+0x3C/+0x7C (the confirmed 6 core attributes), 3 rows at +0x4C/+0x8C
(the still-otherwise-unidentified attribute trio), 8 rows at
+0x58/+0x98, plus one more field. A near-duplicate, differently-
positioned counterpart to the character-sheet screen's own
DrawCharacterStatSheet, used by the training/service screens.
-> DrawTrainingScreenStatSheet

sub_193BE (called from UseItemType_400, right after paying its BCD
material cost, and from UseTrainingItem): the shared interactive
loop -- SelectAndDrawPartyStatusRow + DrawTrainingScreenStatSheet,
draws a remaining-uses count (word_2E38E, colored red at 0) and a
two-tone prompt, then polls keyboard/clicks to let the player pick
another party member and repeat, until uses are exhausted or
cancelled. -> RunItemServiceRecipientLoop

Run via:
    .\run_ida_script.ps1 name_training_stat_sheet_cluster.py
"""
import idc
import ida_name
import ida_bytes

for ea, name, desc in [
    (0x1978F, "DrawStatValueWithCapColor",
     "Draws ax in word_2E412's color if ax<=bx, else word_2E414's "
     "color (an 'over the cap' tint). Generic value-cell renderer, "
     "used throughout DrawTrainingScreenStatSheet."),
    (0x197B9, "DrawTrainingScreenStatSheet",
     "Redraws the current party member's full stat-sheet grid via "
     "DrawStatValueWithCapColor: the 6 core attribute base/derived "
     "pairs (+0x3C/+0x7C), the +0x4C/+0x8C trio, 8 more fields at "
     "+0x58/+0x98, plus one more. Near-duplicate, differently-"
     "positioned counterpart to DrawCharacterStatSheet, used by the "
     "training/service screens. Called from RunItemServiceRecipientLoop "
     "and sub_19553."),
    (0x193BE, "RunItemServiceRecipientLoop",
     "Interactive 'pick a party member, show updated stats, track "
     "remaining uses' loop shared by UseItemType_400's paid-service "
     "branch (called right after paying its BCD material cost) and "
     "UseTrainingItem. Draws DrawTrainingScreenStatSheet + a "
     "remaining-uses count + a two-tone prompt, then polls input to "
     "repeat or exit."),
]:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)
