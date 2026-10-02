"""
Post-load fix-up of a freshly created MM3 database (idat -B -A -c mm3.exe on the unpacked EXE, see
tools/unpack_mm3.py and docs/exe-layout.md).

IDA's MZ loader already understands the Microsoft LINK overlays: it maps the 13 overlays as segments
above the resident image (ovr022..ovr034), turns the overlay stubs in the root (CD 3F thunks) into
`jmp far` thunks (stub022..stub034) and applies the overlay relocations.  What it does not do is find
the code: most routines are only reached through far pointers / the thunks, so almost nothing is
analysed.  This script

  1. gives the segments readable names (ovlNN = overlay NN, stubNN = its thunk segment),
  2. makes ds default to DGROUP everywhere,
  3. seeds functions at every `push bp; mov bp,sp` in every code segment and every thunk target,
  4. lets auto-analysis chase the calls, and
  5. sweeps for code that is still undefined (compiler padding, code only reachable by jump tables).
"""
import idc, idautils, ida_auto, ida_bytes, ida_funcs, ida_segment

DGROUP_SEL = 0x286F
PROLOGUES = (b"\x55\x8B\xEC", b"\x55\x89\xE5")


def seg_bytes(s, e):
    return ida_bytes.get_bytes(s, e - s) or b""


def code_segments():
    for s in idautils.Segments():
        if idc.get_segm_name(s).startswith(("seg0", "ovl", "stub")):
            yield s


# 1. names ---------------------------------------------------------------------------------------
n_ovl = n_stub = 0
for s in list(idautils.Segments()):
    name = idc.get_segm_name(s)
    if name.startswith("ovr"):
        n_ovl += 1
        idc.set_segm_name(s, "ovl%02d" % n_ovl)
    elif name.startswith("stub"):
        n_stub += 1
        idc.set_segm_name(s, "stub%02d" % n_stub)
    elif name == "seg026":
        idc.set_segm_name(s, "stack")
print("overlays:", n_ovl, "stubs:", n_stub)

# 2. ds ------------------------------------------------------------------------------------------
for s in idautils.Segments():
    idc.set_default_sreg_value(s, "ds", DGROUP_SEL)

# 3. seeds ---------------------------------------------------------------------------------------
# The loader's first pass swallows many routines into one huge function (it only follows calls it
# can resolve), so: collect every existing function start plus every prologue, drop all functions,
# and re-create them one by one so each routine ends at the next one.
segs = [(s, idc.get_segm_end(s)) for s in code_segments()]
total = 0
for s, e in segs:
    starts = set(idautils.Functions(s, e))
    data = seg_bytes(s, e)
    for i in range(len(data) - 3):
        if data[i:i + 3] in PROLOGUES:
            starts.add(s + i)
    for f in list(idautils.Functions(s, e)):
        ida_funcs.del_func(f)
    for ea in sorted(starts):
        if not ida_bytes.is_code(ida_bytes.get_flags(ea)):
            idc.create_insn(ea)
        if ida_funcs.add_func(ea):
            total += 1
print("functions after re-seeding:", total)
ida_auto.auto_wait()


# 4./5. sweep the remaining undefined bytes ------------------------------------------------------
def undefined_runs(s, e):
    ea = s
    while ea < e:
        if ida_bytes.is_unknown(ida_bytes.get_flags(ea)):
            st = ea
            while ea < e and ida_bytes.is_unknown(ida_bytes.get_flags(ea)):
                ea += 1
            yield st, ea - st
        else:
            ea += max(idc.get_item_size(ea), 1)


for _ in range(6):
    changed = False
    for s, e in segs:
        for st, n in list(undefined_runs(s, e)):
            ea, stop = st, st + n
            while ea < stop and ida_bytes.get_byte(ea) == 0x90:
                ida_bytes.create_align(ea, 1, 0)
                ea += 1
                changed = True
            if ea < stop and idc.create_insn(ea):
                changed = True
    ida_auto.auto_wait()
    if not changed:
        break

report = []
for s, e in segs:
    left = sum(n for _, n in undefined_runs(s, e))
    nf = len(list(idautils.Functions(s, e)))
    report.append("%-8s %05X-%05X funcs=%4d undefined=%5d/%d" % (idc.get_segm_name(s), s, e, nf, left, e - s))
print("\n".join(report))
print("total functions:", len(list(idautils.Functions())))
