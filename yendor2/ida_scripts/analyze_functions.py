"""
Read-only discovery script: characterizes the 706 still-unnamed (sub_XXXXX)
functions so follow-up naming work (automated or manual) can be targeted --
function size histogram, how many are trivial single-jump/single-call
thunks to an already-named function (safe to bulk-label), and how many
call exactly one DOS/BIOS interrupt with nothing else interesting (their
reapplied comment often already says what they do).

    .\run_ida_script.ps1 analyze_functions.py -NoExport
"""
import idautils
import idc
import ida_funcs
import ida_bytes
import ida_ua

def func_size(ea):
    f = ida_funcs.get_func(ea)
    return f.end_ea - f.start_ea if f else 0

def instr_count(ea):
    f = ida_funcs.get_func(ea)
    if not f:
        return 0
    n = 0
    cur = f.start_ea
    while cur < f.end_ea:
        n += 1
        cur = idc.next_head(cur, f.end_ea)
    return n

unnamed = [ea for ea in idautils.Functions() if idc.get_func_name(ea).startswith("sub_")]
print(f"total functions: {len(list(idautils.Functions()))}, unnamed: {len(unnamed)}")

sizes = [(ea, func_size(ea), instr_count(ea)) for ea in unnamed]

buckets = {"1-2 instr": 0, "3-5 instr": 0, "6-15 instr": 0, "16+ instr": 0}
for ea, sz, n in sizes:
    if n <= 2:
        buckets["1-2 instr"] += 1
    elif n <= 5:
        buckets["3-5 instr"] += 1
    elif n <= 15:
        buckets["6-15 instr"] += 1
    else:
        buckets["16+ instr"] += 1
print("size histogram (by instruction count):", buckets)

# Thunk detection: tiny function whose only real instruction is a
# jmp/call to another function; if that target is named, this one's role
# is fully explained by the target.
thunks = []
for ea, sz, n in sizes:
    if n > 3:
        continue
    f = ida_funcs.get_func(ea)
    cur = f.start_ea
    call_target = None
    ok = True
    while cur < f.end_ea:
        mnem = idc.print_insn_mnem(cur)
        if mnem in ("jmp", "call"):
            tgt = idc.get_operand_value(cur, 0)
            if tgt != idc.BADADDR and ida_funcs.get_func(tgt):
                call_target = tgt
        elif mnem not in ("retn", "retf", "nop", "db"):
            pass  # allow other small setup instructions, just record target if any
        cur = idc.next_head(cur, f.end_ea)
    if call_target is not None:
        tname = idc.get_func_name(call_target)
        thunks.append((ea, call_target, tname, tname.startswith("sub_")))

named_target_thunks = [t for t in thunks if not t[3]]
print(f"\ntiny (<=3 instr) functions that jmp/call a function: {len(thunks)}")
print(f"  ...of which the target is already named (safe thunk-label candidates): {len(named_target_thunks)}")
print("\nsample (ea -> target, target_name):")
for ea, tgt, tname, tgt_unnamed in named_target_thunks[:30]:
    print(f"  {ea:#x} -> {tgt:#x}  {tname}")

# Interrupt-only functions: functions whose body contains an `int` instr
# and whose reapplied comment on that instruction gives a strong signal.
int_only = []
for ea, sz, n in sizes:
    if n > 8:
        continue
    f = ida_funcs.get_func(ea)
    cur = f.start_ea
    int_no = None
    cmt = None
    while cur < f.end_ea:
        if idc.print_insn_mnem(cur) == "int":
            int_no = idc.get_operand_value(cur, 0)
            cmt = idc.get_cmt(cur, 0) or idc.get_cmt(cur, 1)
        cur = idc.next_head(cur, f.end_ea)
    if int_no is not None:
        int_only.append((ea, int_no, cmt))

print(f"\nsmall (<=8 instr) functions containing an int instruction: {len(int_only)}")
for ea, int_no, cmt in int_only[:30]:
    first_line = (cmt or "").split("\n")[0]
    print(f"  {ea:#x}  int {int_no:#x}  {first_line}")
