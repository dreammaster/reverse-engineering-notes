"""
Name the p-machine opcode handlers, dispatch tables and support routines in
wiz1_interp.idb, from docs/pmachine.md. Re-run any time via:

    ida_scripts\\run_ida_script.ps1 -Idb wiz1_interp -ScriptName ida_scripts\\apply_names_interp.py

Idempotent: set_name with SN_FORCE, skips addresses that don't resolve.
"""
import idc
import ida_name
import ida_bytes

# --- core -------------------------------------------------------------------
CORE = {
    0x0000: "interp_start",
    0x001B: "interp_reloc",
    0x00E7: "interp_fetch",          # main fetch/execute loop
    0x00FA: "op_SLDC_push",          # opcode < 0x80 tail
    0x00FD: "op_NOP",
    0x00FF: "pm_fetch_big",          # sub_FF: read 1/2-byte big, return *2
    0x010F: "pm_walk_static_link",   # sub_10F
    0x011A: "pm_init_regs",          # sub_11A
    0x0129: "pm_bootstrap_pascal",   # sub_129: open SYSTEM.PASCAL, set up seg 0
    0x022F: "pm_build_procdict",     # sub_22F
    0x028C: "pm_flush_pending_seg",  # sub_28C
    0x02A1: "off_2A1_dispatch",      # the 128-entry opcode table
    0x0550: "pm_scan_boot_volume",   # sub_550
    0x0619: "pm_open_file",          # sub_619
    0x0639: "pm_load_startup_seg",   # sub_639
    0x0676: "pm_kbd_helper",         # sub_676 (13-case)
    0x07E9: "pm_disk_setup",         # sub_7E9
    0x0924: "pm_read_blocks",        # sub_924
    0x1108: "sbios_disk_io",         # sub_1108: INT 18h AH=18h block I/O
    0x1276: "pm_check_memsize",      # sub_1276: INT 12h, needs >=128K
    0x1307: "pm_hang",               # loc_1307: jmp $
    0x1309: "pm_build_drive_table",  # sub_1309: INT 11h
    0x293E: "pm_proc_entry",         # sub_293E
    0x29FD: "pm_load_code_segment",  # sub_29FD: dynamic segment loader
    0x2ABF: "pm_release_code_segment",  # sub_2ABF
    0x2B40: "op_CSP",                # 0x9E handler
    0x2B4D: "jpt_CSP",               # CSP sub-table
    0x374C: "pm_format_msg",         # sub_374C: printf-ish (fatal + trace)
    0x3716: "pm_trace_init",         # sub_3716
    0x38CB: "pm_trace_step",         # sub_38CB
}

# --- opcode 0x80..0xFF -> (mnemonic, handler_ea) --------------------------
OPS = {
    0x80: ("ABI", 0x2777), 0x81: ("ABR_trap", 0x268C), 0x82: ("ADI", 0x2782),
    0x83: ("ADR_trap", 0x268C), 0x84: ("LAND", 0x2760), 0x85: ("DIF", 0x33E8),
    0x86: ("DVI", 0x27A1), 0x87: ("DVR_trap", 0x268C), 0x88: ("CHK", 0x267A),
    0x89: ("FLO_trap", 0x268C), 0x8A: ("FLT_trap", 0x268C), 0x8B: ("INN", 0x3237),
    0x8C: ("INT_set", 0x3359), 0x8D: ("LOR", 0x2768), 0x8E: ("MODI", 0x27AE),
    0x8F: ("MPI", 0x2799), 0x90: ("MPR_trap", 0x268C), 0x91: ("NGI", 0x278A),
    0x92: ("NGR_trap", 0x268C), 0x93: ("LNOT", 0x2770), 0x94: ("SRS", 0x32C2),
    0x95: ("SBI", 0x2791), 0x96: ("SBR_trap", 0x268C), 0x97: ("SGS", 0x32BF),
    0x98: ("SQI", 0x2657), 0x99: ("SQR_trap", 0x268C), 0x9A: ("STO", 0x3436),
    0x9B: ("IXS", 0x36D8), 0x9C: ("UNI", 0x3394), 0x9D: ("LDE", 0x2630),
    0x9E: ("CSP", 0x2B40), 0x9F: ("LDCN", 0x26A0), 0xA0: ("ADJ", 0x3272),
    0xA1: ("FJP", 0x27D0), 0xA2: ("INC", 0x3180), 0xA3: ("IND", 0x342B),
    0xA4: ("IXA", 0x319D), 0xA5: ("LAO", 0x2706), 0xA6: ("LSA", 0x36A0),
    0xA7: ("LAE", 0x264A), 0xA8: ("MOV", 0x318A), 0xA9: ("LDO", 0x26F7),
    0xAA: ("SAS", 0x36AB), 0xAB: ("SRO", 0x2716), 0xAC: ("XJP", 0x27F5),
    0xAD: ("RNP", 0x28E6), 0xAE: ("CIP", 0x2861), 0xAF: ("EQU", 0x34DA),
    0xB0: ("GEQ", 0x34EE), 0xB1: ("GTR", 0x34F3), 0xB2: ("LDA", 0x2737),
    0xB3: ("LDC", 0x343D), 0xB4: ("LEQ", 0x34E4), 0xB5: ("LES", 0x34E9),
    0xB6: ("LOD", 0x2726), 0xB7: ("NEQ", 0x34DF), 0xB8: ("STR", 0x2749),
    0xB9: ("UJP", 0x27D9), 0xBA: ("LDP", 0x31E7), 0xBB: ("STP", 0x31F9),
    0xBC: ("LDM", 0x3452), 0xBD: ("STM", 0x3465), 0xBE: ("LDB", 0x36F0),
    0xBF: ("STB", 0x36FA), 0xC0: ("IXP", 0x31C3), 0xC1: ("RBP", 0x28D3),
    0xC2: ("CBP", 0x28A4), 0xC3: ("EQUI", 0x3480), 0xC4: ("GEQI", 0x34BC),
    0xC5: ("GTRI", 0x34CB), 0xC6: ("LLA", 0x26C9), 0xC7: ("LDCI", 0x26A6),
    0xC8: ("LEQI", 0x349E), 0xC9: ("LESI", 0x34AD), 0xCA: ("LDL", 0x26BE),
    0xCB: ("NEQI", 0x348F), 0xCC: ("STL", 0x26D5), 0xCD: ("CXP", 0x281A),
    0xCE: ("CLP", 0x284C), 0xCF: ("CGP", 0x288E), 0xD0: ("LPA", 0x31B8),
    0xD1: ("STE", 0x263D), 0xD2: ("NOP2", 0x00FD), 0xD3: ("UNIMPL_D3", 0x2666),
    0xD5: ("SKIP1_D5", 0x2671), 0xD6: ("BPT_HLT", 0x2677),
    0xD8: ("SLDL", 0x26AD), 0xE8: ("SLDO", 0x26E1), 0xF8: ("SIND", 0x3420),
}


def nm(ea, name):
    if ida_bytes.is_mapped(ea):
        ida_name.set_name(ea, name, ida_name.SN_FORCE | ida_name.SN_NOWARN)
        return True
    return False


done = 0
for ea, name in CORE.items():
    if nm(ea, name):
        done += 1
for op, (mnem, ea) in OPS.items():
    if nm(ea, f"op_{mnem}"):
        done += 1
        idc.set_cmt(ea, f"p-code opcode {op:#04x} {mnem}", 0)

print(f"named {done} locations")
