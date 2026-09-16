"""
Names sub_1FBE1, called twice from InitGame -- the installer paired
with the already-named RestoreInt1cVector (which restores the
original INT 1Ch vector from dword_1F97C on shutdown/error-exit).

Initializes several timer-related globals (word_3294C from
word_36CE7, the confirmed animation-speed setting; word_3294E/
word_32950 to 0x5B; word_32952 to 5) and clears a word_3295A flag
bit, then reads the current INT 1Ch (BIOS/DOS user timer tick) vector
via INT 21h/AH=35h, saves it to dword_1F97C, and installs this
session's own handler via INT 21h/AH=25h. -> InstallInt1cTimerHandler

Run via:
    .\run_ida_script.ps1 name_install_int1c_timer_handler.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1FBE1
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "InstallInt1cTimerHandler", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'InstallInt1cTimerHandler': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Saves the original INT 1Ch vector to dword_1F97C (INT 21h/"
    "AH=35h) and installs this session's own handler (INT 21h/"
    "AH=25h), after initializing word_3294C (from word_36CE7, the "
    "animation-speed setting) and related timer globals. Paired "
    "with RestoreInt1cVector. Called twice from InitGame.",
    False,
)
