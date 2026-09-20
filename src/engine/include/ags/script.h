/* ags/script.h -- the compiled-script format and its running
 * instances. Direct port of apply_structs.py's SAFE_DECLS
 * (ccScript/ccInstance). Both structs are FULLY MAPPED -- every byte
 * accounted for, confirmed via a hard malloc-size allocation-site
 * anchor in each case (ccScript: fread_script's `push 1C50h; call
 * malloc`; ccInstance: ccCreateInstanceEx's `push 9A8h; call malloc`).
 * See reversing/notes/csrun-interpreter-evolution.md for the
 * interpreter this struct's `registers[6]`/`pc` drive (the full
 * 38-opcode SCMD_* table -- relevant once M4 in src/PLAN.md is
 * reached).
 */
#ifndef AGS_SCRIPT_H
#define AGS_SCRIPT_H

#include "ags/types.h"

/* ccScript -- one compiled script "module" (the global script, a
 * room script, or a dialog script), as loaded by fread_script. This
 * build fixes what 2011 keeps as separately-malloc'd dynamic arrays
 * (imports/exports/export_addr) at a flat 600-entry capacity each --
 * a genuine 2002 limit, not a later addition 2011 removed. */
struct ccScript {
    char *globaldata;         /* +0x00 */
    int globaldatasize;         /* +0x04 */
    unsigned long *code;          /* +0x08 */
    int codesize;                  /* +0x0C */
    char *strings;                   /* +0x10 */
    int stringssize;                   /* +0x14 */
    char *fixuptypes;                    /* +0x18, TENTATIVE: positional fit against 2011's fixuptypes/fixups/numfixups trio */
    long *fixups;                          /* +0x1C */
    int numfixups;                           /* +0x20 */
    void *imports[600];                       /* +0x24..0x984 */
    int numimports;                             /* +0x984 */
    char *exports[600];                          /* +0x988..0x12E8, function/variable names */
    long export_addr[600];                        /* +0x12E8..0x1C48, packed export-type + offset */
    int numexports;                                 /* +0x1C48 */
    int instances;                                    /* +0x1C4C, refcount, incremented per ccCreateInstanceEx */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct ccScript) == 0x1C50, "ccScript must be 0x1C50 bytes");

/* ccInstance -- one running instance of a ccScript (created via
 * ccCreateInstanceEx). `registers[6]` are this build's own SREG_*
 * general-purpose interpreter registers (2011 declares 8; SREG_OP/
 * SREG_DX are CONFIRMED ABSENT here, since this build's CALL/THISBASE
 * opcodes implement member-function-relative calls without needing a
 * dedicated object register). `exportaddr[600]` is embedded inline
 * (not a separately malloc'd array as in 2011) -- this build's own
 * export-address-resolution step writes computed addresses directly
 * into this array at instance-creation time, never touched again by
 * the interpreter's own bytecode loop afterward. */
struct ccInstance {
    int flags;                       /* +0x00 */
    void *globaldata;                  /* +0x04, malloc'd + memcpy'd (own instance) or shared (fork) */
    int globaldatasize;                  /* +0x08 */
    unsigned long *code;                   /* +0x0C, own malloc'd + memcpy'd copy of ccScript.code */
    int codesize;                            /* +0x10 */
    char *strings;                             /* +0x14, pointer copied directly, not owned/copied */
    int stringssize;                             /* +0x18 */
    void *exportaddr[600];                         /* +0x1C..0x97C */
    char *stack;                                     /* +0x97C, the VM's own data/call stack */
    int stacksize;                                     /* +0x980 */
    int registers[6];                                    /* +0x984, SREG_* -- registers[1]==SREG_SP */
    int pc;                                                /* +0x99C, program counter */
    int line_number;                                        /* +0x9A0, SCMD_LINENUM (opcode 36) target */
    struct ccScript *instanceof_;                             /* +0x9A4 (named instanceof_: `instanceof` is awkward bare) */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct ccInstance) == 0x9A8, "ccInstance must be 0x9A8 bytes");

/* ExecutingScript -- scripts[10], the interpreter's own call-stack
 * array (this build's own architecture gives 5 of 2011's later-unified
 * PostScriptAction cases their own dedicated field: newnum/invscreen/
 * ooo/dlgnum/restartgame -- a later addition in 2011, not a reduced
 * version of something already present). FULLY MAPPED -- zero
 * unaccounted bytes, confirmed end to end via ExecutingScript::init()
 * zero/(-1)-initializing exactly these 8 offsets in this exact order.
 * DRIFT: run_another capacity 2 here vs. 2011's MAX_QUEUED_SCRIPTS=4. */
struct ExecutingScript {
    struct ccInstance *inst;         /* +0x00 */
    int newnum;                        /* +0x04, sentinel -1 = no pending room change */
    int invscreen;                       /* +0x08, 0 = no pending inventory screen */
    int ooo;                               /* +0x0C, pending restore-game slot number */
    int dlgnum;                              /* +0x10, sentinel -1 = no pending dialog */
    char script_run_another[2][30];            /* +0x14..0x50 */
    int run_another_p1[2];                       /* +0x50..0x58 */
    int run_another_p2[2];                         /* +0x58..0x60 */
    int numanother;                                  /* +0x60 */
    int restartgame;                                   /* +0x64, 0 = no pending restart */
    char forked;                                         /* +0x68 */
    char _pad_align2[3];                                   /* +0x69..0x6C, compiler alignment padding */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct ExecutingScript) == 0x6C, "ExecutingScript must be 0x6C bytes");

#endif /* AGS_SCRIPT_H */
