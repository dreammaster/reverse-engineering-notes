"""
Applies retail-game-layer findings to `Shannara.idb` (SHANNARA.LE, the full
game). Source of truth: reversing/analysis/retail_game_layer.md

Run headlessly (IDA GUI must NOT have the .idb open; back it up first):

    idat.exe -A -S"reversing\\scripts\\apply_retail_findings.py" Shannara.idb

Idempotent. A trailing '?' follows the project convention for "working
name, not fully verified". Never overwrites a name that already isn't a
default sub_XXXX name.
"""
import idc

RESULTS = []

# (address, new name, comment)
RENAMES = [
    # --- bit arrays (1-based bit index, LSB-first within each byte) -------
    (0x21F30, "bitset_test",
     "bitset_test(bits=eax, n=dx): 1-based n; returns (bits[(n-1)>>3] >> ((n-1)&7)) & 1; 0 if n==0. "
     "~566 callers (game flags)."),
    (0x21F80, "bitset_set", "bitset_set(bits=eax, n=dx): sets bit n (1-based)."),
    (0x21FC4, "bitset_clear", "bitset_clear(bits=eax, n=dx): clears bit n (1-based)."),
    # --- object table: 5-byte records at byte_20918 {u8 type; u32 ptr}, count in word_217A0 ---
    (0x1A104, "object_attr_locate?",
     "(obj=ax, attr=dx, &bitpos=ebx) -> byte* or 0. Looks the attribute up in the 5-byte attribute "
     "descriptor table at byte_208A0/dword_208A1 ({u8 bitoffset; u32 typeMask}); fails unless the "
     "attribute applies to this object's type ((typeMask >> (type-1)) & 1). Returns the byte holding "
     "the flag and stores the bit position (bitoffset & 7)."),
    (0x1A184, "object_has_attr",
     "object_has_attr(obj=ax, attr=dx): tests one attribute flag bit via object_attr_locate. ~299 callers."),
    (0x1A1B0, "object_set_attr", "object_set_attr(obj=ax, attr=dx)."),
    (0x1A1D8, "object_clear_attr", "object_clear_attr(obj=ax, attr=dx)."),
    (0x1A200, "object_get_location?",
     "object_get_location(obj=ax): returns the word at +0Ah (object types 2,5) or +0Bh (type 3) of the "
     "object's data record, 0 for other types/invalid ids. Likely the parent/container object id."),
    # --- text ---------------------------------------------------------------
    (0x1C0E8, "format_noun_phrase?",
     "Grammar helper called from ~2500 sites. Selector (low 3 bits of arg_0) picks the form: 2 the/some/a, "
     "3 a/an/some, 4 he/she/it/they (or 'you' pronoun form when obj is a player char), 5 your/their/..., "
     "etc. Object id is the upper 16 bits of arg_0 (sar 10h); attribute bits queried through "
     "object_has_attr (1=plural?, 2=?, 3=mass/proper?, 4=vowel?, 16h=female). Compares against "
     "word_ADEB0/word_ADEB2 (player characters) to emit 'your'. Used to build %s inserts such as "
     "'You already have%s.'"),
    # --- command dispatch -----------------------------------------------------
    (0x1B7B0, "run_command?",
     "run_command(verb=eax, dobj=dx, iobj=bx). Re-entrancy counter ADE20, repeat guard CD518 "
     "('Perhaps now is not the best time to try...'), stores the current verb/dobj/iobj in "
     "CD918/CD924/CD92C, then consults per-object handler-existence flags via sub_21AF4(obj,1..6) "
     "before falling back to sub_1E23C(verb) (default response)."),
    (0x26414, "show_dialog?",
     "Large (6 KB) UI routine called ~588 times: hides mouse, sets font 10 / color FF, builds a "
     "text window (push_mouse, sub_82410, sub_7E2EC) from a table at word_BBA0E (6-byte entries). "
     "Working name only."),
    # --- fuzzy matches against the demo (bigram Jaccard >= 0.88, mutual best) ---
    (0x77410, "set_music_volume", "Matches demo set_music_volume (identical mnemonic bigrams)."),
    (0x79370, "unload_pics", "Matches demo unload_pics (identical mnemonic bigrams)."),
    (0x7E024, "reset_mouse_limits", "Matches demo reset_mouse_limits (identical mnemonic bigrams)."),
    (0x95980, "_XMI_read_log?", "Matches demo _XMI_read_log (bigram sim 0.94)."),
    (0x7ED94, "load_font_palette?", "Matches demo load_font_palette (bigram sim 0.91)."),
    (0x15B08, "reset_bevel_style?", "Matches demo reset_bevel_style (bigram sim 0.90)."),
    (0x8517C, "q_kill?", "Matches demo q_kill (bigram sim 0.89)."),
    (0x98960, "AILXMIDI_start_?", "Matches demo AILXMIDI_start_ (bigram sim 0.89)."),
    (0x96660, "_XMI_serve?", "Matches demo _XMI_serve (bigram sim 0.88; also unique string-set match)."),
    (0x776EC, "make_voice?", "Matches demo make_voice (bigram sim 0.86)."),
]


def main():
    for ea, new, cmt in RENAMES:
        cur = idc.get_func_name(ea)
        if not cur:
            RESULTS.append(f"SKIP  {ea:#x}: no function")
            continue
        if not cur.startswith("sub_") and cur != new:
            RESULTS.append(f"SKIP  {ea:#x}: already named {cur!r}, not overwriting")
            continue
        ok = idc.set_name(ea, new, idc.SN_NOWARN)
        idc.set_func_cmt(ea, cmt, 0)
        RESULTS.append(f"{'OK  ' if ok else 'WARN'}  {cur} -> {new} ({ea:#x})")
    print("\n".join(RESULTS))
    idc.save_database(idc.get_idb_path(), 0)


main()

# Headless (idat) has no interactive loop to return to; exit explicitly.
# Inside the GUI (ida.exe) leave the session open.
import ida_kernwin
if not ida_kernwin.is_idaq():
    idc.qexit(0)
