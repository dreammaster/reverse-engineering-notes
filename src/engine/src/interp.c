/* ags/interp.h's own implementation. See that header for the full
 * fidelity/scope notes, and Common/CSRUN.CPP:787-1978 (still present
 * in this repo) for the source this is ported from.
 */
#include "ags/interp.h"
#include "ags/native_api.h"
#include "ags/stub.h"

#include <stdlib.h>
#include <string.h>

/* --- this build's own confirmed register indices (Common/CSCOMP.H;
 * SREG_OP=6/SREG_DX=7 are CONFIRMED ABSENT here, see ags/script.h's
 * own comment on ccInstance.registers[6]). ------------------------- */
#define SREG_SP  1
#define SREG_MAR 2
#define SREG_AX  3
#define SREG_BX  4
#define SREG_CX  5

/* Common/CSCOMP.H's own declared constants. */
#define AGS_CC_STACK_SIZE 4000  /* CC_STACK_SIZE */
#define AGS_CC_MAXNEST 50       /* MAXNEST -- number of same-instance nested SCMD_CALLs allowed */
#define AGS_CC_MAX_FUNC_PARAMS 20

#define FIXUP_GLOBALDATA 1
#define FIXUP_FUNCTION   2
#define FIXUP_STRING     3
#define FIXUP_IMPORT     4
#define FIXUP_DATADATA   5
#define FIXUP_STACK      6
#define EXPORT_FUNCTION  1
#define EXPORT_DATA      2

/* --- SCMD_* opcodes this build's own disassembly confirms (1-38
 * only -- see ags/interp.h's file-level comment). ------------------ */
#define SCMD_ADD          1
#define SCMD_SUB          2
#define SCMD_REGTOREG     3
#define SCMD_WRITELIT     4
#define SCMD_RET          5
#define SCMD_LITTOREG     6
#define SCMD_MEMREAD      7
#define SCMD_MEMWRITE     8
#define SCMD_MULREG       9
#define SCMD_DIVREG       10
#define SCMD_ADDREG       11
#define SCMD_SUBREG       12
#define SCMD_BITAND       13
#define SCMD_BITOR        14
#define SCMD_ISEQUAL      15
#define SCMD_NOTEQUAL     16
#define SCMD_GREATER      17
#define SCMD_LESSTHAN     18
#define SCMD_GTE          19
#define SCMD_LTE          20
#define SCMD_AND          21
#define SCMD_OR           22
#define SCMD_CALL         23
#define SCMD_MEMREADB     24
#define SCMD_MEMREADW     25
#define SCMD_MEMWRITEB    26
#define SCMD_MEMWRITEW    27
#define SCMD_JZ           28
#define SCMD_PUSHREG      29
#define SCMD_POPREG       30
#define SCMD_JMP          31
#define SCMD_MUL          32
#define SCMD_CALLEXT      33
#define SCMD_PUSHREAL     34
#define SCMD_SUBREALSTACK 35
#define SCMD_LINENUM      36
#define SCMD_CALLAS       37
#define SCMD_THISBASE     38

/* Common/CSCOMP.H's own sccmdargs[], truncated to indices 0-38 (this
 * build never emits anything past SCMD_THISBASE). Index is the opcode
 * number itself; value is how many argument words follow it. */
static const short sccmdargs[39] = {
    0, 2, 2, 2, 2, 0, 2,
    1, 1, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 2, 1, 1, 1, 1,
    1, 1
};

/* =====================================================================
 * SystemImports (stub-only -- see ags/interp.h's file-level comment)
 * ===================================================================== */

#define AGS_MAX_STUB_IMPORTS 1024

/* Real (if meaningless) backing memory per fabricated handle, not just
 * a bare fake integer address. Needed because FIXUP_IMPORT doesn't
 * only serve CALLEXT's function-call case -- it's the SAME mechanism
 * this build's compiler uses for imported DATA symbols (character/
 * game/mouse, all real Rob Blanc 1 imports -- see M4's own
 * exploratory notes). A script referencing e.g. `game.foo` loads the
 * import's fixed-up "address" into MAR via LITTOREG, then does
 * ordinary pointer arithmetic (ADD MAR, <struct field offset>)
 * followed by a real MEMREAD/MEMWRITE dereference -- CALLEXT is never
 * involved, so there's no chance to intercept/log it. An unmapped
 * fake address (e.g. 0x00C00000+index) crashes the moment such a
 * script runs; real zeroed heap memory instead reads back as 0/writes
 * harmlessly, exactly matching M4's own "every native call gets
 * stubbed and logged, nothing else needs to work yet" scope. Sized
 * generously (64KB) since at handle-fabrication time we don't know
 * whether this import is a huge struct (GameSetupStructBase is 0xBF84
 * bytes) or a small one (mouse) -- see ags/interp.h. */
#define AGS_STUB_IMPORT_BACKING_SIZE 65536

static const char *s_import_names[AGS_MAX_STUB_IMPORTS];
static void *s_import_handles[AGS_MAX_STUB_IMPORTS];
static int s_import_count = 0;

void ags_system_imports_reset(void)
{
    int i;
    for (i = 0; i < s_import_count; i++) {
        free(s_import_handles[i]);
        s_import_handles[i] = NULL;
    }
    s_import_count = 0;
}

void *ags_system_imports_get_addr_of(const char *name)
{
    int i;
    for (i = 0; i < s_import_count; i++) {
        if (strcmp(s_import_names[i], name) == 0) {
            return s_import_handles[i];
        }
    }
    if (s_import_count >= AGS_MAX_STUB_IMPORTS) {
        return NULL; /* out of fabricated handles -- shouldn't happen for any real script */
    }
    s_import_handles[s_import_count] = calloc(1, AGS_STUB_IMPORT_BACKING_SIZE);
    if (!s_import_handles[s_import_count]) {
        return NULL;
    }
    s_import_names[s_import_count] = name; /* not copied -- see ags/interp.h, the caller's ccScript owns it for the instance's whole lifetime */
    s_import_count++;
    return s_import_handles[s_import_count - 1];
}

const char *ags_system_imports_get_name_of(void *handle)
{
    int i;
    for (i = 0; i < s_import_count; i++) {
        if (s_import_handles[i] == handle) {
            return s_import_names[i];
        }
    }
    return NULL; /* also covers an offset-adjusted DATA-import pointer
                   * (base+fieldoffset) landing here via a CALLEXT that
                   * shouldn't logically happen -- <unresolved import>,
                   * not a crash. */
}

/* =====================================================================
 * ccCreateInstanceEx (single-instance-only port)
 * ===================================================================== */

static void free_instance(struct ccInstance *cinst)
{
    if (!cinst) {
        return;
    }
    free(cinst->globaldata);
    free(cinst->code);
    free(cinst->stack);
    free(cinst);
}

struct ccInstance *ags_cc_create_instance(struct ccScript *scri)
{
    struct ccInstance *cinst;
    long *import_addrs = NULL;
    int i;

    if (!scri) {
        return NULL;
    }

    cinst = (struct ccInstance *)calloc(1, sizeof(struct ccInstance));
    if (!cinst) {
        return NULL;
    }

    cinst->globaldatasize = scri->globaldatasize;
    if (cinst->globaldatasize > 0) {
        cinst->globaldata = malloc((size_t)cinst->globaldatasize);
        if (!cinst->globaldata) {
            free_instance(cinst);
            return NULL;
        }
        memcpy(cinst->globaldata, scri->globaldata, (size_t)cinst->globaldatasize);
    }

    cinst->codesize = scri->codesize;
    if (cinst->codesize > 0) {
        cinst->code = (unsigned long *)malloc((size_t)cinst->codesize * sizeof(long));
        if (!cinst->code) {
            free_instance(cinst);
            return NULL;
        }
        memcpy(cinst->code, scri->code, (size_t)cinst->codesize * sizeof(long));
    }

    cinst->strings = scri->strings; /* shared pointer, not owned/copied -- matches source */
    cinst->stringssize = scri->stringssize;

    cinst->stacksize = AGS_CC_STACK_SIZE;
    cinst->stack = (char *)malloc((size_t)cinst->stacksize);
    if (!cinst->stack) {
        free_instance(cinst);
        return NULL;
    }

    memset(cinst->registers, 0, sizeof(cinst->registers));

    /* Resolve every import to a (stub) address up front. */
    if (scri->numimports > 0) {
        import_addrs = (long *)malloc((size_t)scri->numimports * sizeof(long));
        if (!import_addrs) {
            free_instance(cinst);
            return NULL;
        }
        for (i = 0; i < scri->numimports; i++) {
            char *name = (char *)scri->imports[i];
            if (name == NULL) {
                import_addrs[i] = 0;
                continue;
            }
            import_addrs[i] = (long)(size_t)ags_system_imports_get_addr_of(name);
        }
    }

    /* Apply every fixup (Common/CSRUN.CPP:878-930). FIXUP_IMPORT's own
     * cross-instance CALLEXT->CALLAS rewrite (source's "is_script_
     * import" branch) is NOT ported -- unreachable with no other
     * instance ever registered, see ags/interp.h. */
    for (i = 0; i < scri->numfixups; i++) {
        long fixup = scri->fixups[i];
        if (fixup < 0 || fixup >= cinst->codesize) {
            free(import_addrs);
            free_instance(cinst);
            return NULL;
        }
        switch (scri->fixuptypes[i]) {
        case FIXUP_GLOBALDATA:
            cinst->code[fixup] += (unsigned long)(size_t)cinst->globaldata;
            break;
        case FIXUP_FUNCTION:
            /* no-op -- matches source's own commented-out line */
            break;
        case FIXUP_STRING:
            cinst->code[fixup] += (unsigned long)(size_t)cinst->strings;
            break;
        case FIXUP_IMPORT: {
            long idx = (long)cinst->code[fixup];
            if (idx < 0 || idx >= scri->numimports) {
                free(import_addrs);
                free_instance(cinst);
                return NULL;
            }
            cinst->code[fixup] = (unsigned long)import_addrs[idx];
            break;
        }
        case FIXUP_DATADATA:
            *(long *)((char *)cinst->globaldata + fixup) += (long)(size_t)cinst->globaldata;
            break;
        case FIXUP_STACK:
            cinst->code[fixup] += (unsigned long)(size_t)cinst->stack;
            break;
        default:
            free(import_addrs);
            free_instance(cinst);
            return NULL;
        }
    }
    free(import_addrs);

    /* Resolve every export's own real address (Common/CSRUN.CPP:936-948). */
    for (i = 0; i < scri->numexports; i++) {
        long etype = (scri->export_addr[i] >> 24L) & 0x000ff;
        long eaddr = scri->export_addr[i] & 0x00ffffff;
        if (etype == EXPORT_FUNCTION) {
            cinst->exportaddr[i] = (void *)((char *)cinst->code + eaddr * (long)sizeof(long));
        } else if (etype == EXPORT_DATA) {
            cinst->exportaddr[i] = (void *)((char *)cinst->globaldata + eaddr);
        } else {
            free_instance(cinst);
            return NULL;
        }
    }

    cinst->instanceof_ = scri;
    cinst->pc = 0;
    cinst->flags = 0;
    scri->instances++;

    return cinst;
}

/* =====================================================================
 * cc_run_code (single-instance-only port -- see ags/interp.h)
 * ===================================================================== */

static enum AgsCcRunError ags_cc_run_code(struct ccInstance *inst, long curpc)
{
    long thisbase[AGS_CC_MAXNEST];
    long funcstart[AGS_CC_MAXNEST];
    int curnest = 0;
    long callstack[AGS_CC_MAX_FUNC_PARAMS + 1];
    int callstacksize = 0;
    int was_just_callas = -1;
    long arg1 = 0, arg2 = 0;
    unsigned long thisInstruction;

    inst->pc = curpc;

    if (curpc < 0 || curpc >= inst->codesize) {
        return AGS_CC_RUN_BAD_PC;
    }

    thisbase[0] = 0;
    funcstart[0] = inst->pc;

    for (;;) {
        thisInstruction = inst->code[inst->pc];

        if (inst->pc != inst->codesize - 1) {
            arg1 = (long)inst->code[inst->pc + 1];
            if (inst->pc != inst->codesize - 2) {
                arg2 = (long)inst->code[inst->pc + 2];
            }
        }

        if (thisInstruction < 1 || thisInstruction > SCMD_THISBASE) {
            return AGS_CC_RUN_BAD_OPCODE;
        }

        switch (thisInstruction) {
        case SCMD_LINENUM:
            inst->line_number = (int)arg1;
            break;
        case SCMD_ADD:
            inst->registers[arg1] += (int)arg2;
            if (inst->registers[SREG_SP] - (long)(size_t)inst->stack >= inst->stacksize) {
                return AGS_CC_RUN_STACK_OVERFLOW;
            }
            break;
        case SCMD_SUB:
            inst->registers[arg1] -= (int)arg2;
            break;
        case SCMD_REGTOREG:
            inst->registers[arg2] = inst->registers[arg1];
            break;
        case SCMD_WRITELIT: {
            char *mptr = (char *)(size_t)(unsigned)inst->registers[SREG_MAR];
            memcpy(mptr, &arg2, (size_t)arg1);
            break;
        }
        case SCMD_RET:
            inst->registers[SREG_SP] -= 4;
            curnest--;
            inst->pc = *(long *)(size_t)(unsigned)inst->registers[SREG_SP];
            if (inst->pc == 0) {
                return AGS_CC_RUN_OK;
            }
            continue; /* PC already set to the popped return address */
        case SCMD_LITTOREG:
            inst->registers[arg1] = (int)arg2;
            break;
        case SCMD_MEMREAD:
            inst->registers[arg1] = *(long *)(size_t)(unsigned)inst->registers[SREG_MAR];
            break;
        case SCMD_MEMWRITE:
            *(long *)(size_t)(unsigned)inst->registers[SREG_MAR] = inst->registers[arg1];
            break;
        case SCMD_MULREG:
            inst->registers[arg1] *= inst->registers[arg2];
            break;
        case SCMD_DIVREG:
            if (inst->registers[arg2] == 0) {
                return AGS_CC_RUN_DIVIDE_BY_ZERO;
            }
            inst->registers[arg1] /= inst->registers[arg2];
            break;
        case SCMD_ADDREG:
            inst->registers[arg1] += inst->registers[arg2];
            break;
        case SCMD_SUBREG:
            inst->registers[arg1] -= inst->registers[arg2];
            break;
        case SCMD_BITAND:
            inst->registers[arg1] = inst->registers[arg1] & inst->registers[arg2];
            break;
        case SCMD_BITOR:
            inst->registers[arg1] = inst->registers[arg1] | inst->registers[arg2];
            break;
        case SCMD_ISEQUAL:
            inst->registers[arg1] = (inst->registers[arg1] == inst->registers[arg2]);
            break;
        case SCMD_NOTEQUAL:
            inst->registers[arg1] = (inst->registers[arg1] != inst->registers[arg2]);
            break;
        case SCMD_GREATER:
            inst->registers[arg1] = (inst->registers[arg1] > inst->registers[arg2]);
            break;
        case SCMD_LESSTHAN:
            inst->registers[arg1] = (inst->registers[arg1] < inst->registers[arg2]);
            break;
        case SCMD_GTE:
            inst->registers[arg1] = (inst->registers[arg1] >= inst->registers[arg2]);
            break;
        case SCMD_LTE:
            inst->registers[arg1] = (inst->registers[arg1] <= inst->registers[arg2]);
            break;
        case SCMD_AND:
            inst->registers[arg1] = (inst->registers[arg1] && inst->registers[arg2]);
            break;
        case SCMD_OR:
            inst->registers[arg1] = (inst->registers[arg1] || inst->registers[arg2]);
            break;
        case SCMD_CALL:
            /* Same-instance subroutine call -- no native recursion,
             * just push a return address on the VM's own stack and
             * jump within this same loop iteration (Common/
             * CSRUN.CPP:1464-1494, and this build's own confirmed
             * behavior per matches.json's sub_42B394 entry). */
            if (curnest >= AGS_CC_MAXNEST - 1) {
                return AGS_CC_RUN_CALL_NEST_OVERFLOW;
            }
            *(long *)(size_t)(unsigned)inst->registers[SREG_SP] =
                inst->pc + sccmdargs[thisInstruction] + 1;
            inst->registers[SREG_SP] += 4;

            if (thisbase[curnest] == 0) {
                inst->pc = inst->registers[arg1];
            } else {
                inst->pc = funcstart[curnest] + (inst->registers[arg1] - thisbase[curnest]);
            }
            curnest++;
            thisbase[curnest] = 0;
            funcstart[curnest] = inst->pc;
            if (inst->registers[SREG_SP] - (long)(size_t)inst->stack >= inst->stacksize) {
                return AGS_CC_RUN_STACK_OVERFLOW;
            }
            continue;
        case SCMD_MEMREADB:
            inst->registers[arg1] = *(unsigned char *)(size_t)(unsigned)inst->registers[SREG_MAR];
            break;
        case SCMD_MEMREADW:
            inst->registers[arg1] = *(short *)(size_t)(unsigned)inst->registers[SREG_MAR];
            break;
        case SCMD_MEMWRITEB:
            *(unsigned char *)(size_t)(unsigned)inst->registers[SREG_MAR] = (unsigned char)inst->registers[arg1];
            break;
        case SCMD_MEMWRITEW:
            *(short *)(size_t)(unsigned)inst->registers[SREG_MAR] = (short)inst->registers[arg1];
            break;
        case SCMD_JZ:
            if (inst->registers[SREG_AX] == 0) {
                inst->pc += arg1;
            }
            break;
        case SCMD_PUSHREG:
            *(long *)(size_t)(unsigned)inst->registers[SREG_SP] = inst->registers[arg1];
            inst->registers[SREG_SP] += 4;
            if (inst->registers[SREG_SP] - (long)(size_t)inst->stack >= inst->stacksize) {
                return AGS_CC_RUN_STACK_OVERFLOW;
            }
            break;
        case SCMD_POPREG:
            inst->registers[SREG_SP] -= 4;
            inst->registers[arg1] = *(long *)(size_t)(unsigned)inst->registers[SREG_SP];
            break;
        case SCMD_JMP:
            /* No INSTF_RUNNING hung-script check here -- a 2.56-era
             * addition, CONFIRMED ABSENT from this build (see
             * ags/interp.h's file-level comment). */
            inst->pc += arg1;
            break;
        case SCMD_MUL:
            inst->registers[arg1] *= (int)arg2;
            break;
        case SCMD_CALLEXT: {
            /* Native/imported function call. num_args_to_func/
             * next_call_needs_object (2011's own locals) are always
             * at their reset values in THIS build, since the only
             * opcodes that ever set them -- SCMD_NUMFUNCARGS(39) and
             * SCMD_CALLOBJ(45) -- are both CONFIRMED ABSENT (outside
             * this build's own confirmed 1-38 opcode range). This
             * means numparm is always exactly callstacksize, and the
             * "member function call" special case never applies --
             * see ags/interp.h's file-level comment. */
            void *handle = (void *)(size_t)(unsigned)inst->registers[arg1];
            const char *name = ags_system_imports_get_name_of(handle);
            /* ags/native_api.h -- M11's own "remaining script-API
             * entries" slice: dispatches to a real handler when one
             * exists, otherwise falls back to the exact same
             * ags_stub_hit logging this case always did. */
            inst->registers[SREG_AX] = ags_native_api_call(name, callstack, callstacksize);
            was_just_callas = -1;
            break;
        }
        case SCMD_PUSHREAL:
            if (callstacksize >= AGS_CC_MAX_FUNC_PARAMS) {
                return AGS_CC_RUN_STACK_OVERFLOW;
            }
            callstack[callstacksize] = inst->registers[arg1];
            callstacksize++;
            break;
        case SCMD_SUBREALSTACK:
            if (was_just_callas >= 0) {
                inst->registers[SREG_SP] -= arg1 * 4;
                was_just_callas = -1;
            }
            callstacksize -= (int)arg1;
            break;
        case SCMD_CALLAS: {
            /* Call into an exported function -- this build's own
             * confirmed architecture recurses natively into THIS
             * SAME function (see ags/interp.h's file-level comment),
             * single-instance-only: the target must be within this
             * SAME instance's own code. */
            long callAddr;
            long oldstack;
            long oldpc;
            enum AgsCcRunError rc;
            int aa;

            for (aa = 0; aa < callstacksize; aa++) {
                *(long *)(size_t)(unsigned)inst->registers[SREG_SP] = callstack[aa];
                inst->registers[SREG_SP] += 4;
            }
            *(long *)(size_t)(unsigned)inst->registers[SREG_SP] = 0; /* sentinel return address */
            oldstack = inst->registers[SREG_SP];
            inst->registers[SREG_SP] += 4;
            if (inst->registers[SREG_SP] - (long)(size_t)inst->stack >= inst->stacksize) {
                return AGS_CC_RUN_STACK_OVERFLOW;
            }

            oldpc = inst->pc;
            callAddr = inst->registers[arg1] - (long)(size_t)inst->code;
            if (callAddr % 4 != 0) {
                return AGS_CC_RUN_BAD_PC;
            }
            callAddr /= 4;

            rc = ags_cc_run_code(inst, callAddr);
            if (rc != AGS_CC_RUN_OK) {
                return rc;
            }
            if (oldstack != inst->registers[SREG_SP]) {
                return AGS_CC_RUN_STACK_OVERFLOW; /* "stack corrupt after function call" */
            }
            inst->pc = oldpc;
            was_just_callas = callstacksize;
            break;
        }
        case SCMD_THISBASE:
            thisbase[curnest] = arg1;
            break;
        /* SHIFTLEFT(43)/SHIFTRIGHT(44)/MODREG(40)/XORREG(41)/NOTREG(42)
         * are all opcodes >38 -- CONFIRMED ABSENT from this build (see
         * ags/interp.h's file-level comment); the bounds check above
         * already rejects them before this switch is ever reached, so
         * there's deliberately no case for them here. */
        default:
            return AGS_CC_RUN_BAD_OPCODE;
        }

        inst->pc += sccmdargs[thisInstruction] + 1;
    }
}

/* =====================================================================
 * ccCallInstance (simplified -- see ags/interp.h)
 * ===================================================================== */

int ags_cc_call_instance(struct ccInstance *inst, const char *funcname, int numargs,
                          const long *args, long *out_return_value)
{
    long startat = -1;
    int k;
    long tempstack[AGS_CC_MAX_FUNC_PARAMS + 1];
    int tssize;
    long stoffs;
    enum AgsCcRunError rc;

    if (numargs > AGS_CC_MAX_FUNC_PARAMS || numargs < 0) {
        return AGS_CC_RUN_BAD_OPCODE; /* mirrors ccCallInstance's own "too many arguments" -- reusing an existing error code rather than adding a one-off */
    }
    if (inst->pc != 0) {
        return AGS_CC_RUN_ALREADY_RUNNING;
    }

    /* Exact-match only -- this build's own compiled scripts store
     * plain export names, not 2011's later "name$argcount" mangled
     * form (confirmed directly against Rob Blanc 1's own real
     * compiled global script, see src/PLAN.md's M4). */
    for (k = 0; k < inst->instanceof_->numexports; k++) {
        char *thisExportName = inst->instanceof_->exports[k];
        if (thisExportName && strcmp(thisExportName, funcname) == 0) {
            long etype = (inst->instanceof_->export_addr[k] >> 24L) & 0x000ff;
            if (etype != EXPORT_FUNCTION) {
                return AGS_CC_RUN_FUNC_NOT_FOUND;
            }
            startat = inst->instanceof_->export_addr[k] & 0x00ffffff;
            break;
        }
    }
    if (startat < 0) {
        return AGS_CC_RUN_FUNC_NOT_FOUND;
    }

    tempstack[0] = 0; /* sentinel return address */
    for (tssize = 1; tssize <= numargs; tssize++) {
        tempstack[tssize] = args[tssize - 1];
    }
    numargs++; /* account for the return address */

    stoffs = 0;
    for (tssize = numargs - 1; tssize >= 0; tssize--) {
        memcpy(&inst->stack[stoffs], &tempstack[tssize], sizeof(long));
        stoffs += (long)sizeof(long);
    }
    inst->registers[SREG_SP] = (int)(size_t)inst->stack;
    inst->registers[SREG_SP] += numargs * (int)sizeof(long);

    rc = ags_cc_run_code(inst, startat);

    inst->registers[SREG_SP] -= (numargs - 1) * (int)sizeof(long);
    if (out_return_value) {
        *out_return_value = inst->registers[SREG_AX];
    }
    inst->pc = 0;

    return (int)rc;
}
