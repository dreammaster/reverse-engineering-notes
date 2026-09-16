"""
Names sub_150B8, called once from ShowClueBook (at +0x43, before the
already-named RestoreClueBookBackgroundFromEMS is called later at
+0x4B5) -- the exact save counterpart of that restore function.

Maps EMS page 0x5616 (the same dedicated clue-book full-screen page),
then rep movsw's 0x7D00 words FROM the real VGA screen (ds=0xA000)
INTO the EMS page frame (es=_emsSegmentPageFrame) -- the mirror image
of RestoreClueBookBackgroundFromEMS's EMS-to-_videoBufferSeg copy.
Saves whatever's on screen before the F8 clue book UI draws over it,
so it can be restored when the clue book closes.
-> SaveClueBookBackgroundToEMS

Run via:
    .\run_ida_script.ps1 name_save_cluebook_background.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x150B8
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SaveClueBookBackgroundToEMS", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SaveClueBookBackgroundToEMS': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Saves the entire VGA screen (0xA000) to EMS page 0x5616 -- the "
    "save counterpart of RestoreClueBookBackgroundFromEMS, called "
    "earlier in ShowClueBook before the clue book UI draws over the "
    "screen. Called only from ShowClueBook.",
    False,
)
