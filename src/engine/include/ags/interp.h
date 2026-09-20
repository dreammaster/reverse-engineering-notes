/* ags/interp.h -- M4 ("Run the interpreter on nothing", see
 * src/PLAN.md): ccCreateInstanceEx + ccCallInstance + cc_run_code,
 * ported from Common/CSRUN.CPP:787-1978 (all three still present in
 * this repo) and narrowed to exactly the 38 opcodes this build's own
 * disassembly confirms it implements (reversing/analysis/
 * matches.json's sub_42B394 entry: a bounds check admits only
 * opcodes 1-38; every one matches Common/CSCOMP.H's declared SCMD_*
 * semantics with zero drift). Opcodes 39-72 (the entire float-
 * arithmetic block, the managed/dynamic-pointer block, CALLOBJ,
 * CHECKBOUNDS, LOADSPOFFS, CREATESTRING, STRINGSEQUAL/NOTEQ,
 * LOOPCHECKOFF, JNZ) are deliberately NOT implemented here -- this
 * build's own compiler never emits them, so a real script can never
 * contain one; hitting one is treated as a decode error, not
 * silently ignored.
 *
 * Architectural notes (this build vs. the 2011 reference source,
 * already independently confirmed by disassembly -- see the same
 * matches.json entry for the full writeup):
 *   - No `runningInst` field, no loadedInstances[] table, no
 *     cross-instance CALLEXT->CALLAS rewriting: this build's own
 *     ccInstance (ags/script.h) has none of that, and this port is
 *     SINGLE-INSTANCE-ONLY as a direct consequence -- CALLAS can only
 *     target the SAME instance's own code here. Multi-instance
 *     support (a room script calling into the global script, etc.)
 *     is later milestone work, once something other than "load and
 *     run the global script alone" needs it.
 *   - No call-stack bookkeeping arrays in ccInstance (2011's
 *     callStackLineNumber/Addr/CodeInst/Size) -- nested same-instance
 *     calls (SCMD_CALL) are handled by pushing a return address onto
 *     the VM's OWN data stack and continuing the SAME dispatch loop
 *     (no native recursion); only SCMD_CALLAS recurses natively into
 *     this same function.
 *   - No INSTF_RUNNING hung-script detection in SCMD_JMP (a 2.56-era
 *     addition, after this build's own 2.4b pin) -- backward jumps
 *     are unconditional, no iteration-count safety net.
 *
 * SystemImports (this build's own confirmed `simp` global) is
 * reimplemented here as a STUB-ONLY registry for M4's own stated
 * purpose: rather than requiring every native function to already be
 * registered (and failing instance creation on any that aren't, like
 * the real ccCreateInstanceEx does), ags_system_imports_get_addr_of()
 * lazily fabricates a unique handle for ANY name -- so instance
 * creation always succeeds, and every native call the script actually
 * makes gets logged via AGS_STUB the first time it happens (see
 * SCMD_CALLEXT's own handling in interp.c). This is a deliberate,
 * documented divergence from the original engine's strict behavior,
 * not an oversight.
 */
#ifndef AGS_INTERP_H
#define AGS_INTERP_H

#include "ags/script.h"

/* --- SystemImports (stub-only, see the file-level comment above) -- */
void ags_system_imports_reset(void);
void *ags_system_imports_get_addr_of(const char *name);
const char *ags_system_imports_get_name_of(void *handle);

/* --- ccCreateInstanceEx (Common/CSRUN.CPP:787-972), single-instance-
 * only (see the file-level comment above). Every FIXUP_* type this
 * build's own script compiler emits (GLOBALDATA/FUNCTION/STRING/
 * IMPORT/DATADATA/STACK) is applied exactly per source, EXCEPT
 * FIXUP_IMPORT's own cross-instance CALLEXT->CALLAS rewrite (not
 * reachable with no other instance ever registered). Returns a
 * malloc'd struct ccInstance* (caller owns it), or NULL on failure. */
struct ccInstance *ags_cc_create_instance(struct ccScript *scri);

enum AgsCcRunError {
    AGS_CC_RUN_OK = 0,
    AGS_CC_RUN_BAD_PC = -1,         /* cc_error's own "specified code offset is not valid" */
    AGS_CC_RUN_DIVIDE_BY_ZERO = -2, /* "!Integer divide by zero" */
    AGS_CC_RUN_BAD_OPCODE = -3,     /* "invalid instruction %d found in code stream" (includes every opcode >=39, see the file-level comment) */
    AGS_CC_RUN_STACK_OVERFLOW = -4,
    AGS_CC_RUN_CALL_NEST_OVERFLOW = -5,
    AGS_CC_RUN_FUNC_NOT_FOUND = -6, /* ccCallInstance's own "function '%s' not found" */
    AGS_CC_RUN_ALREADY_RUNNING = -7 /* ccCallInstance's own "instance already being executed" */
};

/* --- ccCallInstance (Common/CSRUN.CPP:1879-1978), simplified: looks
 * up `funcname` in inst->instanceof_->exports[] via an EXACT match
 * only (this build's own compiled scripts store plain names, not
 * 2011's later "name$argcount" mangled form -- confirmed directly
 * against Rob Blanc 1's own real compiled global script), pushes
 * `numargs` long-sized arguments plus a sentinel 0 return address
 * onto the instance's own stack, and runs the interpreter from the
 * export's own code offset. *out_return_value receives the script
 * function's own final AX register on a normal return (may be NULL
 * if the caller doesn't need it). Returns AGS_CC_RUN_OK or a negative
 * AgsCcRunError. */
int ags_cc_call_instance(struct ccInstance *inst, const char *funcname, int numargs,
                          const long *args, long *out_return_value);

#endif /* AGS_INTERP_H */
