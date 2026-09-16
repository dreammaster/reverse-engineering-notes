"""
SUPERSEDED by fix_cursor_naming.py in the same round: reading Fade?'s
full body afterward showed sub_23A22's draw/erase role was backwards
(it restores saved background = erases, doesn't draw) and found the
real draw function (renamed from Fade? instead). Kept for the record of
how the investigation actually went; see fix_cursor_naming.py for the
corrected names and reasoning.

Names the mouse cursor blit and its dirty-flag gate, found by tracing
RunGameDialog's 'A' (Animation) handler sub_1F163 -> sub_237B0 ->
sub_23A22:

- sub_23A22: blits a 16x8-per-row tile (looped to 16 rows = 16x16) from
  a fixed source (ds:si=0xE0E -- the same source address "Fade?" uses
  for a tile blit, strongly suggesting a shared cursor sprite buffer)
  to the video buffer at a pixel position computed directly from
  word_2E776 (x) and word_31956 (y) (`320*y + x`, not going through
  getTextPos -- consistent with pixel-granular cursor movement, not
  text-cell-granular). -> DrawMouseCursor
- sub_237B0: gates on two dirty-flag bits in word_3195C before calling
  DrawMouseCursor -- i.e. "redraw the cursor only if it's actually
  moved/needs it". -> RefreshMouseCursor

Run via:
    .\run_ida_script.ps1 name_mouse_cursor.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x23A22: "DrawMouseCursor",
    0x237B0: "RefreshMouseCursor",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x23A22,
    "Blits the 16x16 mouse cursor sprite (source ds:0xE0E) to the video "
    "buffer at pixel position (word_2E776, word_31956) -- 320*y+x, "
    "computed directly rather than via getTextPos (pixel-granular, not "
    "text-cell).",
    False,
)
ida_bytes.set_cmt(
    0x237B0,
    "Redraws the mouse cursor (DrawMouseCursor) only if the dirty flags "
    "in word_3195C (bits 0 and 1) indicate it's needed. Called very "
    "widely (effectively a per-frame/per-event cursor-service call).",
    False,
)
