"""
Names sub_28B94, called repeatedly from DrawIndentedTextColumn --
the actual word-drawing primitive: draws one word from a
pre-formatted text buffer where NUL bytes mark line ends (not just
string end).

Skips leading spaces (advancing _textPos_x by 6 each), writes
non-space characters one at a time via writeChar until hitting a
space or NUL, then skips trailing spaces the same way. If that
skip run hits NUL: resets _textPos_x to the line's left margin (dx),
advances _textPos_y by 6, decrements cx (the caller's remaining-lines
counter) -- this word was the end of a pre-wrapped line. If it hits
another non-space character instead: returns without decrementing cx
-- more words follow on the same line, and the caller (DrawIndentedTextColumn)
loops back to draw them. In short: text is pre-wrapped into
NUL-separated lines by the caller/data, and this function just walks
word-by-word through one line at a time, detecting line boundaries by
NUL rather than doing width-based wrapping itself.
-> DrawWordToken

Run via:
    .\run_ida_script.ps1 name_draw_word_token.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x28B94
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawWordToken", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawWordToken': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws one word from [bx] (skipping leading/trailing spaces, "
    "writeChar per character). On hitting NUL: resets _textPos_x to "
    "dx, advances _textPos_y, decrements cx (line-end). On hitting "
    "another word: returns without decrementing cx (same line "
    "continues). Text is pre-wrapped by NUL line separators, not "
    "wrapped here. Called from DrawIndentedTextColumn.",
    False,
)
