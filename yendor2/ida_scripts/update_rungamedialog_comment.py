"""
Corrects the comment on sub_2AE3C (the item-icon command dispatcher
documented earlier this session, NOT RunGameDialog -- double-checked
the xref before writing this): its 0x242-0x245 shared handler,
sub_294A3, is now resolved as UseAbilityOnTarget (a discovery mechanic:
try commands on the object you're facing until you find the one that
unlocks it), not the "character panel" guess made when this comment was
first written (that guess was explicitly noted as not confirmed).

Run via:
    .\run_ida_script.ps1 update_rungamedialog_comment.py
"""
import ida_bytes

ida_bytes.set_cmt(
    0x2AE3C,
    "Command dispatcher on word_32974 (event/command code), covering "
    "0x242-0x2C8. 0x242-0x245 (4 codes) share one handler, "
    "UseAbilityOnTarget -- a discovery mechanic (try the current "
    "command against whatever object the player is facing; the right "
    "one permanently unlocks it). 0x246-0x249 (4 codes) each flash an "
    "icon then a screen transition, or a generic 'unavailable' flash if "
    "sub_27A5E(0xB1) capability check fails -- plausibly the manual's "
    "4 single-key inventory item icons (disk/keyring/map/hourglass) but "
    "the specific code<->item mapping isn't confirmed. 0x26D and 0x2C8 "
    "are single one-off codes. See "
    "ida_scripts/document_item_icon_dispatch.py for the original trace.",
    False,
)
print("done")
