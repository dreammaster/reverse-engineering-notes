"""
Names sub_14DFC, called only from ShowClueBook: restores the entire
video buffer (0x7D00 words = full VGA screen) from its own dedicated
EMS page (0x5616, distinct from the 0x55D8 page used by the
portrait/full-screen-restore cluster named earlier this session) --
i.e. restores whatever was on screen before the F8 clue book opened.
-> RestoreClueBookBackgroundFromEMS

Run via:
    .\run_ida_script.ps1 name_cluebook_bg_restore.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x14DFC
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RestoreClueBookBackgroundFromEMS", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RestoreClueBookBackgroundFromEMS': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Restores the entire video buffer from EMS page 0x5616 (a "
    "dedicated page, distinct from 0x55D8) -- the screen behind the F8 "
    "clue book, restored when it closes. Called only from ShowClueBook.",
    False,
)
