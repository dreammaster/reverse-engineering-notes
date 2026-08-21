"""
Read-only diagnostic: Paul flagged a known bug in the custom tool that
flattened the original RTLink-linked gatemain executable into
gatemain_decoded.exe for disassembly -- when a far call/jmp targets
another routine within the SAME original RTLink overlay segment (so it
doesn't go through the rtlink_thunk cross-segment gate), the tool has
sometimes failed to correctly patch that far pointer's SEGMENT word
during flattening. The offset word should still be reliable even when
broken this way.

This re-examines the 243 "same owning function" cases excluded from the
712 real RTLink thunks (find_rtlink_thunks.py/
apply_rtlink_thunks_gatemain.py) to check whether they're genuine
split/tail-chunked functions, or instances of this exact flattening bug
producing a bogus tiny "segment" value that happens to parse as a
suspiciously small/stale-looking chunk address.

    .\\run_ida_script.ps1 -Idb gatemain -ScriptName diagnose_thunk_chunks.py -NoExport
"""

import idc
import idautils
import ida_bytes
import ida_funcs

rtlink_thunk_ea = idc.get_name_ea_simple("rtlink_thunk")
print(f"rtlink_thunk = {rtlink_thunk_ea:#x}")

cases = []
for ea in idautils.Functions():
    fname = idc.get_func_name(ea)
    if not fname.startswith("sub_"):
        continue
    end = idc.get_func_attr(ea, idc.FUNCATTR_END)
    size = end - ea
    if size > 12 or size < 5:
        continue
    if ida_bytes.get_byte(ea) != 0xE8:
        continue
    if rtlink_thunk_ea not in idautils.CodeRefsFrom(ea, 0):
        continue
    jmp_ea = ea + 3
    targets = list(idautils.CodeRefsFrom(jmp_ea, 0))
    if not targets:
        continue
    target_ea = targets[0]
    if idc.get_func_attr(target_ea, idc.FUNCATTR_START) != ea:
        continue
    cases.append((ea, jmp_ea, target_ea))

print(f"\n{len(cases)} same-owning-function cases found\n")

# Full scan: flag any case whose far-jmp doesn't decode as a clean
# EA-opcode direct far jump with a plausible (non-tiny) segment value,
# or whose target isn't valid decoded code -- candidates for Paul's
# described flattening bug (unpatched segment word on an intra-segment
# far call). Real segments elsewhere in this binary run at least into
# the low thousands (paragraph) upward, so treat anything under 0x100
# as suspicious.
suspicious = []
clean = 0
for ea, jmp_ea, target_ea in cases:
    raw = [ida_bytes.get_byte(jmp_ea + i) for i in range(5)]
    opcode = raw[0]
    if opcode != 0xEA:
        suspicious.append((ea, jmp_ea, target_ea, f"opcode={opcode:#x} not far-jmp-direct"))
        continue
    seg = raw[3] | (raw[4] << 8)
    if seg < 0x100:
        suspicious.append((ea, jmp_ea, target_ea, f"suspiciously small encoded segment {seg:#x}"))
        continue
    if not idc.is_code(ida_bytes.get_full_flags(target_ea)):
        suspicious.append((ea, jmp_ea, target_ea, "target is not decoded as code"))
        continue
    clean += 1

print(f"full scan of all {len(cases)}: {clean} clean (plausible split function), "
      f"{len(suspicious)} suspicious\n")
for ea, jmp_ea, target_ea, reason in suspicious[:30]:
    print(f"  SUSPICIOUS thunk={ea:#x} jmp_ea={jmp_ea:#x} target={target_ea:#x}: {reason}")

print()
for ea, jmp_ea, target_ea in cases[:15]:
    raw = [ida_bytes.get_byte(jmp_ea + i) for i in range(5)]
    opcode = raw[0]
    print(f"thunk={ea:#x} jmp_ea={jmp_ea:#x} target={target_ea:#x} "
          f"raw_bytes={' '.join(f'{b:02X}' for b in raw)} opcode={opcode:#x}")
    if opcode == 0xEA:
        off = raw[1] | (raw[2] << 8)
        seg = raw[3] | (raw[4] << 8)
        print(f"    far jmp direct: encoded offset={off:#06x} encoded segment={seg:#06x} (para {seg:#x} = byte addr {seg*16:#x})")
    elif opcode == 0xE9:
        rel = raw[1] | (raw[2] << 8)
        if rel >= 0x8000:
            rel -= 0x10000
        print(f"    near jmp rel16: {rel:+#x} -> {jmp_ea+3+rel:#x}")
    else:
        print(f"    unexpected opcode {opcode:#x}, insn={idc.GetDisasm(jmp_ea)!r}")

    # what does IDA think is at/around the target?
    tgt_fname = idc.get_func_name(target_ea)
    tgt_disasm = idc.GetDisasm(target_ea)
    print(f"    target label={idc.get_name(target_ea)!r} func={tgt_fname!r} disasm={tgt_disasm!r}")

    # chunk info for the thunk's function
    fn = ida_funcs.get_func(ea)
    nchunks = 0
    if fn:
        it = ida_funcs.func_tail_iterator_t(fn)
        ok = it.main()
        while ok:
            chunk = it.chunk()
            print(f"    chunk: {chunk.start_ea:#x}-{chunk.end_ea:#x}")
            nchunks += 1
            ok = it.next()
    print()
