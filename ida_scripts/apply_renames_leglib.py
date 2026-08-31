"""
IDA Pro script: renames for leglib.idb (LEGLIB.EXE) -- the shared BASIC
run-time module. resolve_rtm_leglib.py names every int-3Fh entry
`rtm_<key>` (provisional); this file replaces the confidently-identified
ones with real names. resolve_rtm_leglib.py won't clobber a non-`rtm_`
name on re-run, so run this after it, then re-run resolve_rtm_leglib.py
once to refresh rtm_map.py (the client `-> <name>` comments read from it).

Identifications are from the routine bodies + how menu.idb / out.idb call
them (arg counts, call context). `bas*` = stock Microsoft BASIC 6
run-time; unprefixed = LEGLIB's own engine layer (the FE/FF ordinals,
themselves compiled BASIC -- they open with `call rtm_F0`).

    .\run_ida_script.ps1 -Idb leglib -ScriptName apply_renames_leglib.py
"""

import sys
import os
import idc
import ida_funcs
import ida_auto

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rtm_map import RTM_MAP

# key (as in rt_<key>) -> (name, note)
RENAMES = {
    "F0": ("basProcEnter",
           "SUB/FUNCTION prologue: pops the return addr to ds:738h, saves "
           "si/di/cx/ax, `sub sp,cx` for the local frame (cx = frame size, "
           "set by the caller just before), links the frame via ds:116h, "
           "`inc ds:118h` (nesting depth). Every compiled-BASIC proc -- "
           "including LEGLIB's own FE/FF engine routines -- opens with "
           "`mov cx,N / call basProcEnter`."),
    "F4": ("basProcLeave",
           "SUB/FUNCTION epilogue: `dec ds:118h`, restores the frame, "
           "`jmp [ds:738h]` back to the caller. Pairs with basProcEnter; "
           "proc bodies end `call basProcLeave / retf 2`."),
    "EC": ("basProcExit1",
           "first half of a proc's outer exit wrapper (clears ds:132h). "
           "menu SUB tails are `call basProcExit1 / jmp basProcExit2`. "
           "[verify: event/line-trap hook vs. return path]"),
    "ED": ("basProcExit2",
           "second half of the proc exit wrapper -- checks ds:136h and "
           "returns to the caller. The `jmp` target of every menu.idb "
           "SUB epilogue (IDA daisy-chain-names the stubs j_j_..._rt_ED)."),
    "C2": ("basStrAssign",
           "string assignment: copy the 4-byte string descriptor from "
           "src to dst. args (dst, src) on the stack; the single most "
           "common run-time call (164x in out, 171x in menu)."),
    "C3": ("basStrConcat",
           "string concatenation (`a$ + b$`): sums the two operand "
           "lengths, allocates, copies both. args (a, b)."),
    "D1": ("basStrClear",
           "zero a string descriptor (free a temp / `LET a$ = \"\"`). "
           "arg (descriptor)."),
    "73": ("basStrBuild",
           "string-space copy+pad helper (`rep movsb` then `rep stosw`); "
           "used while materialising a string result."),
    "00": ("basArrayCopy",
           "block copy between two far buffers -- args "
           "(word, dst:dword, count, src:dword). Array / FIELD move."),
    "CE": ("basPlayMusic",
           "start playing an MML music string (`PLAY`): stashes the "
           "string ptr at ds:7A4h. menu's showTitleScreen alternates "
           "this with playMusicTick over the 5 theme strings."),
    "AF": ("basScreenInit",
           "multi-entry with rtm_0F (AL = 0 vs 0FFh mode flag); called "
           "twice at every module's entry with rect-ish args. Screen / "
           "text-window setup. [name provisional]"),
    "FE26": ("drawString",
             "engine: draw one position-coded screen string. arg (string); "
             "body is `basProcEnter / drawStringInner / screenRefresh / "
             "basProcLeave`."),
    "FE25": ("drawStringInner",
             "engine: parse + emit a position-coded string (copies it "
             "local via basStrAssign, walks it, writes cells)."),
    "FE5B": ("screenRefresh",
             "engine: post-draw cursor/row update (divides a packed "
             "row*256+col by 100h). [name provisional]"),
    "FE2A": ("drawTileRun",
             "draw a horizontal run of 8x8 CGA cells. args "
             "(srcBase, srcSeg, destOff, tileIndexList, count). For each "
             "of `count` cells: al = *tileIndexList++; if al==0FFh skip "
             "(transparent), else copy the 16-byte cell at "
             "srcBase + al*16 to video via sub_1FED8 (movsw x8, "
             "+4Eh steps, +1F0Eh field-jump after the 4th -- the same "
             "field-interleaved 8x8 layout as the .GLB tile sheets). "
             "dun/mus's blitViewCell drives this for every wall/floor "
             "band."),
    "FE2E": ("andSpriteMaskCell",
             "sprite MASK pass: AND one field-interleaved 8x8 cell "
             "(8 words) into video (`and es:[di],ax` x8, +4Eh / +1F0Eh "
             "stepping). args (srcOff, srcSeg, destOff). drawViewSprite "
             "calls it once per masked cell, walking a list of "
             "(srcOff,destOff) word pairs -- exactly DUNOBJ.BSV's "
             "pointer-pair table."),
    "FE63": ("resolveAndOpenGameFile",
             "resolve a game data-file's path -- which drive holds it, per "
             "DRCONFIG.DAT -- and OPEN it. Called before every BLOAD and "
             "before random-file GET/PUT (dun's loadDungeonData, saver's "
             "readCharDat / saveRosterToDisk). arg &name$."),
    "FE07": ("basBload",
             "the BASIC `BLOAD name$, offset` statement: thin wrapper over "
             "rtm_02 -- read the 7-byte `[FD][seg:2][off:2][len:2]` BSAVE "
             "header then the payload, into the target array/segment. "
             "args (&name$, &destOffset). dun's loadDungeonData uses it for "
             "DUNM*/DUNDATA/DUNOBJ; celdrv for the CEL/DIS frames."),
    "02": ("basBloadRaw",
           "BLOAD core: OPEN, read `[FD][seg][off][len]` (7 bytes) into "
           "ds:651h, then read `len` bytes to the caller's dest (arg_0!=0) "
           "or to the header's own seg:off. args (useOverride, destOff, seg)."),
    "61": ("basPutSprite",
           "stock Microsoft BASIC `PUT (x,y), array, verb` -- draws a "
           "GET-array bitmap (`dw widthPx ; dw heightPx ; planar CGA "
           "rows`). ds:0E92h = bits/pixel (2 for CGA), ds:250h/252h = "
           "origin, ds:2D0h/2D1h = H-flip flags, ds:0EA7h/0EABh = the "
           "AND vs OR row-drawer. drawViewSprite uses this for the "
           "sprite IMAGE after andSpriteMaskCell lays the mask."),
    "60": ("basPutSpriteXor",
           "sibling of basPutSprite (rtm_61) -- the other `PUT` verb "
           "(PSET/XOR path); shares sub_1BE5C / sub_1BEF0."),
}


def main():
    done = miss = 0
    for key, (name, note) in RENAMES.items():
        prefix = None
        if key.startswith(("FE", "FF")) and len(key) == 4:
            prefix, ordv = int(key[:2], 16), int(key[2:], 16)
        else:
            ordv = int(key, 16)
        ent = RTM_MAP.get((prefix, ordv))
        if not ent:
            print(f"  [!] {key}: not in rtm_map")
            miss += 1
            continue
        ea = ent["ea"]
        cur = idc.get_name(ea)
        if cur != name:
            idc.set_name(ea, name, idc.SN_NOWARN)
        idc.set_func_cmt(ea, note, 1)
        done += 1
    ida_auto.auto_wait()
    print(f"applied {done}, missing {miss}")


main()
