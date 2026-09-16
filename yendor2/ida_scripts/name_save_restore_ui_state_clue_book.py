"""
Names the matched save/restore pair used by ShowClueBook to snapshot
a large chunk of global UI/display state before opening the
full-screen clue book overlay, and restore it afterward.

sub_14F92 (called once from ShowClueBook, right before entering the
clue book): computes a small fixed size, allocMem's a scratch block,
stores its segment into `fe`, then serializes ~45 global words
(video position/font colors, EMS pointers, the current item/spell/
selection state words_2E5xx/2E54x/31946-3194C, the word_328Cx/3290x/
3293x/3296x/3297x UI flag words, etc.) into the block via `stosw`,
followed by several fixed-size `rep movsw` block copies from other
data tables. -> SaveUiStateForClueBook

sub_14E28 (called once from ShowClueBook, in its cleanup path after
freeing the clue-book-local `fe` block and popping the outer one):
reads `fe`, then `lodsw`s the exact same ~45 words back out in the
same order into the same globals, the precise mirror of
SaveUiStateForClueBook. -> RestoreUiStateForClueBook

Run via:
    .\run_ida_script.ps1 name_save_restore_ui_state_clue_book.py
"""
import idc
import ida_name
import ida_bytes

for ea, name, desc in [
    (0x14F92, "SaveUiStateForClueBook",
     "Allocates a scratch memory block (handle stored in `fe`) and "
     "serializes ~45 global UI/display-state words plus several "
     "fixed-size data table copies into it via stosw/rep movsw. "
     "Called once from ShowClueBook before opening the clue book "
     "overlay. Paired with RestoreUiStateForClueBook."),
    (0x14E28, "RestoreUiStateForClueBook",
     "Reads the scratch block referenced by `fe` and lodsw's the "
     "same ~45 global UI/display-state words back out, in the same "
     "order SaveUiStateForClueBook wrote them. Called once from "
     "ShowClueBook's cleanup path."),
]:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)
