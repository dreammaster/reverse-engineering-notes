"""
Round 1 of global-variable renaming: the highest-reference-count,
single-clear-meaning globals, each confirmed by existing
file-formats.md documentation or direct disassembly evidence.

- byte_2E400 -> g_lastKeyChar: the raw character/scan-code from the
  last PollKeyboardInput poll (uppercased a-z, or the extended
  scan-code byte); also doubles as a "quit" sentinel (0xFF).
- word_2E530 -> g_pictureId: the picture id within the currently
  selected category, read by DrawPicture (confirmed by several
  pre-existing comments: "id=word_2E530, category 0x80" /
  "id from word_2E530/word_2E532").
- word_2E532 -> g_pictureCategory: the g_pictureDir byte-offset
  selecting which picture category/directory sub-table DrawPicture
  looks the id up in (confirmed: "looks up g_pictureDir[word_2E532]").
- word_328D4 -> g_currentPartyRecord: the currently-selected party
  record's base address, set by SelectPartyRecordById as
  (id-1)*0x1F4 + 0x95F3 (confirmed in file-formats.md).
- word_328D6 -> g_currentPartyRecordId: the 1-based character id
  SelectPartyRecordById caches alongside g_currentPartyRecord.
- word_32924 -> g_selectedPartySlotPtr: a pointer into
  g_partySlotAssignment (which of the 4 slot addresses), resolved to
  a character id and then a full record via SelectPartyRecordById
  (confirmed in file-formats.md).
- word_32974 -> g_currentActionId: the current command/item/ability
  id, dispatched on by HandleGameCommand and fed to
  LoadItemCatalogRecord -- confirmed this session that item ids and
  ability/command ids share one numbering space.

Run via:
    .\run_ida_script.ps1 rename_globals_round1.py -NoExport
"""
import idc
import ida_name

RENAMES = [
    (0x2E400, "g_lastKeyChar"),
    (0x2E530, "g_pictureId"),
    (0x2E532, "g_pictureCategory"),
    (0x328D4, "g_currentPartyRecord"),
    (0x328D6, "g_currentPartyRecordId"),
    (0x32924, "g_selectedPartySlotPtr"),
    (0x32974, "g_currentActionId"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
