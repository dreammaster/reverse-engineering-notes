"""
Names sub_1B8EE and sub_1B818, called from UseItem and FinishItemUse:
draws a filtered, 2-column list of eligible catalog items.

sub_1B818 (0x1B818): given an item catalog record (es:si, stride 0x3A),
checks its category flags (`[+0x16]`/`[+0x18]`) against a filter pair
(word_2E40C/word_2E40E), then 6 prerequisite flag ids at `[+0x22..+0x2C]`
via the already-named TestGlobalFlag (positive id = must be set,
negative = must be clear). If every check passes (errorCode=0), copies
the item's name (`+0x0`, 14 bytes -- the confirmed name field) into the
display buffer 0xAFA8, also showing ShowMaterialCounterHud when
`[+0x18]` bit 2 is set. -> CheckItemEligibilityAndCopyName

sub_1B8EE (0x1B8EE): iterates up to `word_2E432` catalog entries
(stride 0x3A) calling CheckItemEligibilityAndCopyName on each; for
every eligible one, draws its name and records the catalog index in a
result buffer (0xBB8, max 10 entries), laid out in 2 columns of 5 rows
(text_x switches at the 5th match). -> DrawEligibleItemList

Run via:
    .\run_ida_script.ps1 name_eligible_item_list.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x1B818, "CheckItemEligibilityAndCopyName"),
    (0x1B8EE, "DrawEligibleItemList"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1B818,
    "Checks an item catalog record's category flags ([+0x16]/[+0x18]) "
    "against word_2E40C/word_2E40E, then 6 prerequisite flag ids at "
    "[+0x22..+0x2C] via TestGlobalFlag. If all pass, copies the item's "
    "name into 0xAFA8 (errorCode=0) and shows ShowMaterialCounterHud if "
    "[+0x18] bit 2 is set. Called from DrawEligibleItemList.",
    False,
)
ida_bytes.set_cmt(
    0x1B8EE,
    "Iterates up to word_2E432 item catalog entries, drawing the name "
    "of each one CheckItemEligibilityAndCopyName approves, in a 2-column "
    "x 5-row layout, and recording each match's catalog index in a "
    "result buffer (0xBB8, max 10). Called from UseItem and "
    "FinishItemUse.",
    False,
)
