"""
IDA Pro script: master list of symbol renames for out.idb (OUT.EXE) --
the overworld / towns / dungeons engine (chains to MUS.EXE, SAVER.EXE,
TWNDR.EXE, CASDR.EXE, DUN.EXE).

Names-only, run last:
  resolve_thunks -> coerce_code -> resolve_thunks -> apply_renames_out
(coerce_code does the structural work + fall-through crefs; this only
sets names + repeatable comments and must not trigger a reanalysis.)

Add (ea, name, note) entries as functions become clear. seg000 is
~99.7% coerced (2026-08-30); 190 functions, most still sub_, a handful
over-merged by the call-far fragmentation merge -- flagged for a later
tuning pass. OUT's DGROUP screen text is position-coded and not readily
readable, so naming will lean more on the rtm_* call pattern and the
data-file / chained-EXE references than on printed strings.

    .\run_ida_script.ps1 -Idb out -ScriptName apply_renames_out.py
"""

import idc
import idautils
import ida_segment

DRY_RUN = False

# (ea, new_name, note)
#
# OUT has no readable inline text (DGROUP is position-coded), so names
# come from the call graph + rtm_* pattern + the `ds:` state vars each
# function pokes. Confident structural names only so far; the ~90 helpers
# still need per-function work. Recurring shapes seen while surveying:
#   * `mov word ds:21XXh, <const>` runs  -> engine-parameter presets
#     (coordinate/size tables around ds:2156h..ds:2178h)
#   * `mov word ds:2234h, <mask> ; call sub_11760`  -> set a game/quest
#     flag (sub_11638 / 11681 / 1168A / 116D8 / 11705 / 1171F family)
#   * `mov word ds:2192h,N ; mov word ds:1F04h,M`  -> menu-option wiring
#     (sub_121F7 .. sub_12245)
#   * `mov word ds:2146h, 1|2|3 ; jmp j_rt_FE4E`  -> mode select
#   * `mov ax, 2Bxxh ; push ax`  -> push a screen-string address
#   ds:1F2Ah is the top-level mode var (overworld / town / dungeon / …);
#   sub_12969 and sub_117B0 both dispatch on it.
RENAMES = [
    (0x10030, "out_entry",
     "OUT.EXE entry / module init: declares the module-scope variables "
     "(14x basValuePush/basValueStore via rt_FF4B/rt_FF50) and sets up "
     "the screen (rt_AF x3, rt_98). Falls through into outInit."),

    (0x10199, "outInit",
     "overworld first-time setup: 9x basScreenInit (screen regions), then "
     "loads the overworld data via the engine (rt_FE63/FE44/FE16/FE68/"
     "FE37) and calls sub_10B06. ~2 KB, called once from out_entry."),

    (0x13C60, "mainDispatch",
     "the central overworld command/event loop -- a large (~3.5 KB) "
     "function called from ~15 sites (often at big `+offset`s into "
     "itself). Branches on ds:1F2Ah and fans out to the per-command "
     "helpers (movement, look, board, the parameter setters, etc.)."),

    (0x12969, "updateGameState",
     "dispatches on ds:1F2Ah (`mov ax, ds:1F2Ah / and ax,ax` then a "
     "jump table) -- the mode switch between overworld / town / dungeon "
     "/ combat states."),

    (0x11760, "applyGameFlag",
     "shared tail of the ds:2234h flag-setter family (sub_11638 etc.): "
     "`mov si, 1B96h` then folds the pushed mask into the flag word."),

    (0x11638, "setFlag_03", "set game flag mask 0x03 (-> applyGameFlag)."),
    (0x11681, "setFlag_38", "set game flag mask 0x38 (-> applyGameFlag)."),
    (0x1168A, "setFlag_C0", "set game flag mask 0xC0 (-> applyGameFlag)."),
    (0x116D8, "setFlag_0300", "set game flag mask 0x0300 (-> applyGameFlag)."),
    (0x11705, "setFlag_0800", "set game flag mask 0x0800 (-> applyGameFlag)."),
    (0x1171F, "setFlag_1000", "set game flag mask 0x1000 (-> applyGameFlag)."),

    (0x127D2, "setMode_1", "ds:2146h := 1, then jmp j_rt_FE4E."),
    (0x127DB, "setMode_2", "ds:2146h := 2, then jmp j_rt_FE4E."),
    (0x127E4, "setMode_3", "ds:2146h := 3."),
]


def main():
    seg = ida_segment.get_segm_by_name("seg000")
    S0, S0E = seg.start_ea, seg.end_ea

    done = skip = 0
    for ea, name, note in RENAMES:
        cur = idc.get_name(ea)
        if DRY_RUN:
            print(f"  {ea:#x}: {cur!r} -> {name!r}")
            continue
        if cur != name and not idc.set_name(ea, name, idc.SN_NOWARN):
            print(f"  [!] set_name failed at {ea:#x} -> {name}")
            continue
        idc.set_func_cmt(ea, note, 1)
        done += 1 if cur != name else 0
        skip += 1 if cur == name else 0

    total = sum(1 for _ in idautils.Functions(S0, S0E))
    named = sum(1 for f in idautils.Functions(S0, S0E)
                if not idc.get_func_name(f).startswith(("sub_", "j_", "nullsub")))
    print(f"\napplied {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))
    print(f"seg000: {named}/{total} functions named")


main()
