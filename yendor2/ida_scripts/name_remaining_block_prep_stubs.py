"""
Names the remaining LoadMasterPalette-family resource-block-setup
stubs, each identified by its single already-named caller.

- sub_27C78 (InitializeNewGameWorldState, table 0xCE6F, size 0x1388)
  -> PrepareNewGameResetBlockRead
- sub_27DA8 (RunGameDialog + confirmed via SaveCurrentGameToSlot's
  own "master-header records" description, table 0xCDD3, size
  0x1388 -- same size as the new-game-reset stub, a shared header
  record shape) -> PrepareMasterHeaderBlockRead
- sub_27D8A (RunGameDialog, table 0xCDEB, size 0x30C0)
  -> PrepareGameDialogLargeBlockRead
- sub_27E04 (RunGameDialog, table 0xCDE7, size from caller's cx)
  -> PrepareGameDialogIndexedBlockRead
- sub_27E20 (RunGameDialog, table 0xCDD7, size from word_329FA)
  -> PrepareGameDialogSizedBlockRead
- sub_27E3A (LoadGroundItemSlotRecord, ReadGroundItemSlot, table
  0xCDDB, size 0x22) -> PrepareGroundItemSlotBlockRead
- sub_27E7C/9A/B8/D6 (loadWorldDat1, 4 sequential calls at +0x75/
  +0x95/+0xB5/+0xD5, sizes 0x10/0xC/8/0xC)
  -> PrepareWorldDat1Block1Read..Block4Read
- sub_27EF4 (loadWorldDat2, size 0x1A) -> PrepareWorldDat2BlockRead
- sub_27F12 (loadWorldDat3, size 4) -> PrepareWorldDat3BlockRead

Run via:
    .\run_ida_script.ps1 name_remaining_block_prep_stubs.py
"""
import idc
import ida_name
import ida_bytes

desc = "Resource-block-setup stub (LoadMasterPalette family). Called from {caller}."

entries = [
    (0x27C78, "PrepareNewGameResetBlockRead", "InitializeNewGameWorldState"),
    (0x27DA8, "PrepareMasterHeaderBlockRead", "RunGameDialog and SaveCurrentGameToSlot"),
    (0x27D8A, "PrepareGameDialogLargeBlockRead", "RunGameDialog"),
    (0x27E04, "PrepareGameDialogIndexedBlockRead", "RunGameDialog"),
    (0x27E20, "PrepareGameDialogSizedBlockRead", "RunGameDialog"),
    (0x27E3A, "PrepareGroundItemSlotBlockRead", "LoadGroundItemSlotRecord and ReadGroundItemSlot"),
    (0x27E7C, "PrepareWorldDat1Block1Read", "loadWorldDat1"),
    (0x27E9A, "PrepareWorldDat1Block2Read", "loadWorldDat1"),
    (0x27EB8, "PrepareWorldDat1Block3Read", "loadWorldDat1"),
    (0x27ED6, "PrepareWorldDat1Block4Read", "loadWorldDat1"),
    (0x27EF4, "PrepareWorldDat2BlockRead", "loadWorldDat2"),
    (0x27F12, "PrepareWorldDat3BlockRead", "loadWorldDat3"),
]

for ea, name, caller in entries:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc.format(caller=caller), False)
