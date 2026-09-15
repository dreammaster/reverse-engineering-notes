"""
Documents (comment only) sub_2AE3C's dispatch table, traced while
following up on the text-drawing cluster from rank_naming_candidates.py.
Dispatches on word_32974 (a command/event code):
  - 0x242-0x245 (4 sequential codes, one shared handler sub_294A3):
    plausibly the manual's "F1-F4 character statistic panel" hotkeys,
    but sub_294A3 turned out to be an ESC-cancelable list/menu selection
    routine (scans a table at 0xDFBB for a match), not simply "draw
    panel N" -- the character-panel hypothesis is NOT confirmed, don't
    rename sub_294A3 from it.
  - 0x246-0x249 (4 sequential codes): each flashes a small icon (via
    sub_23B76) then a screen transition (Fade?), or falls back to a
    generic "unavailable" flash if a capability check (sub_27A5E(0xB1))
    fails. The manual (docs/manual.txt) lists exactly 4 single-key
    inventory-item icons in this vein: "D" disk icon, "K" keyring,
    "M" party map, "T" hourglass. Strongly suggests sub_2B029/sub_2AFB8/
    sub_2B09A (+ a 4th, possibly sub_2AF2E) are those 4 icons, but WHICH
    code maps to WHICH item isn't determinable from static analysis
    alone (would need to correlate each one's icon-graphic ID against
    PICTURES.VGA's actual layout) -- not renaming from a 1-in-4 guess.
  - 0x26D: calls ShutdownAudioDrivers-adjacent sub_2849C then a command
    to sub_28296.
  - 0x2C8: calls sub_2B17F.

Run via:
    .\run_ida_script.ps1 document_item_icon_dispatch.py
"""
import ida_bytes

ida_bytes.set_cmt(
    0x2AE3C,
    "Command dispatcher on word_32974 (event/command code), covering "
    "0x242-0x2C8. 0x242-0x245 (4 codes) share one handler (sub_294A3, "
    "an ESC-cancelable list-selection routine). 0x246-0x249 (4 codes) "
    "each flash a small icon then a screen transition, or a generic "
    "'unavailable' flash if sub_27A5E(0xB1) capability check fails -- "
    "plausibly the manual's 4 single-key inventory item icons (disk/"
    "keyring/map/hourglass) but the specific code<->item mapping isn't "
    "confirmed. 0x26D and 0x2C8 are single one-off codes. See "
    "ida_scripts/document_item_icon_dispatch.py for the full trace.",
    False,
)
print("done")
