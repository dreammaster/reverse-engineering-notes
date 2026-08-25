"""
IDA Pro script: corrects a stale function comment on SPACE.EXE's
execWithEnvp, flagged in roadmap.md for double-checking.

The comment (written by an earlier session, before this function's
callee was traced/named) reads: "exec?() itself is presumably the
real DOS EXEC (INT 21h AH=4Bh) child-process spawn/overlay-load
implementation; not traced further here". That guess predates
execProgramEntry being named -- the callee is now confirmed (same name,
same byte-identical shape as OUT.EXE/ULTIMA.EXE/GEN.EXE's own
execProgramEntry, all traced in earlier sessions this project) to be
the **custom overlay loader**, not real DOS INT 21h/4Bh EXEC: it reads
the target file directly, builds a PSP by hand, and far-JMPs into the
loaded image without ever returning to DOS.

Traced execWithEnvp's only caller, SPACE.EXE's own `exit()` (the
"leave outer space" function): saves `_savegame` to "inuse.u1", then
calls execWithEnvp("out.exe", "S", 0), retrying via
promptDiskSwapRetry on failure -- the exact same
save-then-chain-with-retry pattern confirmed everywhere else in this
project. So the "(DOS EXEC...)" phrasing was loose/outdated wording
from before the mechanism was traced, not a genuine architectural
difference, settling the open roadmap question.

    .\\run_ida_script.ps1 -Idb ultima1_space -ScriptName fix_execwithenvp_comment.py
"""

import idc

DRY_RUN = False

EA = None  # resolved below via name lookup

OLD_COMMENT = (
    "Thin wrapper adding the current envp before forwarding to exec?() - "
    "exec?() itself is presumably the real DOS EXEC (INT 21h AH=4Bh) "
    "child-process spawn/overlay-load implementation; not traced further "
    "here as it's generic C-runtime process-exec plumbing rather than "
    "save/load logic. Called by exit()."
)

NEW_COMMENT = (
    "Thin wrapper adding the current envp before forwarding to "
    "execProgramEntry -- confirmed (2026-08-25) to be the same custom "
    "overlay loader used everywhere else in this project (reads the "
    "target file directly, builds a PSP by hand, far-JMPs in -- never "
    "real DOS INT 21h/4Bh EXEC, despite this comment's earlier guess). "
    "Called by exit() (SPACE.EXE's own 'leave outer space' function): "
    "saves _savegame to inuse.u1, then execWithEnvp(\"out.exe\", \"S\", 0), "
    "retrying via promptDiskSwapRetry on failure."
)


def main():
    ea = idc.get_name_ea_simple("execWithEnvp")
    if ea == idc.BADADDR:
        print("[!] execWithEnvp not found -- aborting")
        return
    cur = idc.get_func_cmt(ea, 0)
    if cur == NEW_COMMENT:
        print(f"{ea:#x}: already updated -- skipping")
        return
    if cur != OLD_COMMENT:
        print(f"[!] current comment doesn't match expected stale text -- aborting to avoid clobbering something else")
        print(f"    current: {cur!r}")
        return
    print(f"{ea:#x}: replacing stale comment")
    print(f"  old: {cur!r}")
    print(f"  new: {NEW_COMMENT!r}")
    if DRY_RUN:
        print("[dry] not applied")
        return
    ok = idc.set_func_cmt(ea, NEW_COMMENT, 0)
    print(f"  set_func_cmt ok={ok}")


if __name__ == "__main__":
    main()
