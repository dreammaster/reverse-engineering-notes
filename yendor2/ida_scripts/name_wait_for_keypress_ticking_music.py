"""
Names sub_162B6, called from 19 sites across the codebase -- a core
"wait for a keypress, ticking ambient music each iteration" building
block, distinct from the larger, separately-named PollKeyboardInput.

Loops: calls UpdateAmbientMusic, then polls for a key via
INT 21h AH=6 DL=0xFF (non-blocking direct console I/O). If a key
arrived, stores it in byte_2E400 and proceeds to cleanup. If not,
checks word_3195C bits 0x2400 as a loop-continue gate (if neither bit
is set, loops back for another poll); otherwise handles two more
paths -- clearing byte_2E400 (bit 0x2000 set, skips forcing ESC) or
forcing byte_2E400=0x1B/ESC and checking bit 0x400 to decide whether
to loop again or fall through. Cleanup clears bits from word_3195C
and flushes the DOS keyboard buffer (INT 21h AH=0xC) before returning.
In short: a blocking key-wait loop with ambient-music ticking and a
couple of flag-gated early-exit/synthetic-ESC conditions.
-> WaitForKeypressTickingMusic

Run via:
    .\run_ida_script.ps1 name_wait_for_keypress_ticking_music.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x162B6
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "WaitForKeypressTickingMusic", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'WaitForKeypressTickingMusic': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Loops calling UpdateAmbientMusic then polling for a key "
    "(INT 21h AH=6). On a key, stores it in byte_2E400. On no key, "
    "loops again unless word_3195C bits 0x2400 gate an early exit "
    "(clearing byte_2E400, or forcing it to ESC/0x1B per bit 0x2000/"
    "0x400). Flushes the DOS keyboard buffer before returning. Called "
    "from 19 sites across the codebase.",
    False,
)
