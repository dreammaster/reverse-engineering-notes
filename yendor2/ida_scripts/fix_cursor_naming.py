"""
CORRECTION to name_mouse_cursor.py from earlier this round: reading
Fade?'s full body (previously only read a middle fragment) shows the
draw/erase roles were backwards.

The full picture, now that both functions are read start-to-end:
  - 0xE0E is a "saved background" buffer -- whatever was on screen
    under the cursor before it was drawn there.
  - 0x3FE6 is the actual cursor sprite bitmap (blitted with 0xFF as a
    transparent color key).
  - sub_23A22 (named DrawMouseCursor last round) copies FROM 0xE0E TO
    the video buffer -- that's RESTORING the saved background, i.e.
    ERASING the cursor, not drawing it. Renaming to
    RestoreCursorBackground.
  - sub_237B0 (named RefreshMouseCursor) gates calling sub_23A22 on
    dirty flags -- i.e. "erase the cursor if needed". Renaming to
    RestoreCursorBackgroundIfDirty.
  - Fade? actually does the real draw: saves the new position's
    background INTO 0xE0E (for the next erase), then blits the cursor
    sprite from 0x3FE6 (with 0xFF transparency) onto the video buffer
    at (word_2E776, word_31956). Renaming to DrawMouseCursor.

This also resolves the original "Fade?" hedge from the prior IDA-8.3-era
session: it was never a screen fade, it's the mouse cursor draw routine
-- explains why it's called so pervasively throughout the binary (once
per cursor move).

Run via:
    .\run_ida_script.ps1 fix_cursor_naming.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x23A22: "RestoreCursorBackground",
    0x237B0: "RestoreCursorBackgroundIfDirty",
    0x2A11B: "DrawMouseCursor",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x23A22,
    "Restores the saved background (0xE0E) to the video buffer at "
    "(word_2E776, word_31956) via getTextPos -- erases the cursor from "
    "its last drawn position. (Corrected from an earlier, backwards "
    "'DrawMouseCursor' name this session -- see fix_cursor_naming.py.)",
    False,
)
ida_bytes.set_cmt(
    0x237B0,
    "Calls RestoreCursorBackground (erasing the cursor) only if the "
    "dirty flags in word_3195C (bits 0 and 1) indicate it's needed. "
    "Called very widely (effectively a per-frame/per-event cursor "
    "service call).",
    False,
)
ida_bytes.set_cmt(
    0x2A11B,
    "The real mouse-cursor draw: if word_3195C bit1 is set, first saves "
    "the video buffer content at the cursor's new position into 0xE0E "
    "(so RestoreCursorBackground can erase it later), then blits the "
    "cursor sprite from 0x3FE6 onto the video buffer with 0xFF as a "
    "transparent color key. Not a screen fade despite the inherited "
    "name/hedge -- explains why it's called so pervasively (once per "
    "cursor move).",
    False,
)
