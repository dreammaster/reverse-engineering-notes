"""
Names the Set/Test accessor pair for the +0xCA per-record flag bank,
matching the naming convention already established for the sibling
+0x10C bank (GetRecordFlagBitAndWord_10C/SetRecordFlag_10C/
TestRecordFlag_10C).

sub_27A4E (called from UseTrainingItem, sub_25456, and others): ORs
the bit mask from GetRecordFlagBitAndWord_CA into [si] -- sets the
flag. -> SetRecordFlag_CA

sub_27A66 (called from BuildAlchemySpellList and
MarkIneligiblePartyMembers): tests [si] & the bit mask from
GetRecordFlagBitAndWord_CA (ZF result) -- checks the flag.
-> TestRecordFlag_CA

Run via:
    .\run_ida_script.ps1 name_record_flag_ca_accessors.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x27A4E: "SetRecordFlag_CA",
    0x27A66: "TestRecordFlag_CA",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x27A4E,
    "Sets a bit in the +0xCA per-record flag bank: ORs "
    "GetRecordFlagBitAndWord_CA's mask into [si]. Called from "
    "UseTrainingItem, sub_25456, and others.",
    False,
)
ida_bytes.set_cmt(
    0x27A66,
    "Tests a bit in the +0xCA per-record flag bank: [si] & "
    "GetRecordFlagBitAndWord_CA's mask (ZF result). Called from "
    "BuildAlchemySpellList and MarkIneligiblePartyMembers.",
    False,
)
