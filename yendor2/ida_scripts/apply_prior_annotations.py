"""
Reapplies the tentative function/variable naming, comments, structures, and
function-prototype work from the IDA-8.3-era database (lost when the .idb
had to be recreated from scratch under IDA 8.2 after a machine move -- see
docs/overview.md "Headless IDA pipeline") to the current, freshly
auto-analyzed 8.2 database.

Source data:
  - ida_scripts/backups/yendor2.idc.pre-8.2-rebuild.bak
      Full IDC export from the old (8.3) database. Kept as the durable
      reference/backup. NOT replayed directly: idat.exe -c fresh-reload +
      compile_idc_file()+main() was tried first and turned out to silently
      drop the vast majority of the renaming work (the old Functions_0()
      is one enormous IDC function -- ~750 add_func/set_name/set_cmt calls
      back to back -- and the classic IDC VM appears to choke partway
      through it with no reported error: a scratch test reproduced only
      45 of 748 names this way).
  - ida_scripts/backups/yendor2_annotations.pre-8.2-rebuild.json
      What this script actually applies: the individual set_name/set_cmt/
      SetType calls, pulled out of the .bak file by regex (see
      scratchpad's extract_idc_annotations.py used to generate it) and
      applied here one at a time via the Python API, so each item's
      success/failure is visible instead of the whole batch silently
      failing together.

Since both databases are analysis of the same input file (SW.EXE, same
md5), addresses line up directly -- no fuzzy matching needed.

Run via:
    .\run_ida_script.ps1 apply_prior_annotations.py
"""
import json
import os

import ida_bytes
import ida_name
import ida_typeinf
import idc

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ANNOTATIONS_PATH = os.path.join(
    SCRIPT_DIR, "backups", "yendor2_annotations.pre-8.2-rebuild.json"
)

# Struct layouts, hand-transcribed from the old .idc's Structures_0() /
# yendor2.asm (member names and sizes only -- the old .idc's raw
# add_struc_member() flags/typeid args reference netnode ids from the old
# database, which are meaningless to replay directly).
STRUCT_DECLS = r"""
struct FontChar
{
  char data[6];
};

struct FileEntry
{
  __int16 _handle;
  __int16 _bufferSeg;
  __int16 _buffer;
  __int16 _blockSize;
  __int16 _blockIndex;
  __int16 _blockOffset;
  __int16 _blockOffsetHi;
};

struct Struc1
{
  __int16 field_0;
  __int16 field_2;
  __int16 field_4;
};

struct StringXY
{
  __int16 x;
  __int16 y;
  char *str;
};
"""


def apply_structs():
    til = ida_typeinf.get_idati()
    errs = ida_typeinf.parse_decls(til, STRUCT_DECLS, None, ida_typeinf.HTI_DCL)
    return errs


def apply_names(names):
    ok, failed, unchanged, deltail_ok = 0, [], 0, 0
    for ea_str, name in names.items():
        ea = int(ea_str, 16)
        if not ida_bytes.is_mapped(ea):
            failed.append((ea, name, "address not mapped"))
            continue
        existing = idc.get_name(ea)
        if existing == name:
            unchanged += 1
            continue
        res = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
        if res:
            ok += 1
            continue
        # Likely a tail byte inside an item whose boundaries the 8.2
        # auto-analysis drew differently than the 8.3-era analysis (e.g. one
        # data blob vs several individually-labeled words). SN_DELTAIL
        # splits the hindering item so the label can land where the old
        # analysis had it -- matches the old, more fine-grained layout.
        res = ida_name.set_name(
            ea, name,
            ida_name.SN_NOWARN | ida_name.SN_FORCE | ida_name.SN_DELTAIL,
        )
        if res:
            deltail_ok += 1
        else:
            failed.append((ea, name, "set_name failed (incl. SN_DELTAIL retry)"))
    return ok, unchanged, deltail_ok, failed


def apply_comments(comments):
    ok, failed = 0, []
    for ea_str, c in comments.items():
        ea = int(ea_str, 16)
        if not ida_bytes.is_mapped(ea):
            failed.append((ea, c["text"], "address not mapped"))
            continue
        res = ida_bytes.set_cmt(ea, c["text"], c["repeatable"])
        if res:
            ok += 1
        else:
            failed.append((ea, c["text"], "set_cmt failed"))
    return ok, failed


def apply_types(types):
    ok, failed = 0, []
    for ea_str, decl in types.items():
        ea = int(ea_str, 16)
        if not ida_bytes.is_mapped(ea):
            failed.append((ea, decl, "address not mapped"))
            continue
        res = idc.SetType(ea, decl)
        if res:
            ok += 1
        else:
            failed.append((ea, decl, "SetType failed (parse or apply error)"))
    return ok, failed


def main():
    with open(ANNOTATIONS_PATH, "r", encoding="utf-8") as f:
        data = json.load(f)

    print(f"loaded {len(data['names'])} names, {len(data['comments'])} comments, "
          f"{len(data['types'])} types from {ANNOTATIONS_PATH}")

    struct_errs = apply_structs()
    print(f"apply_structs: parse_decls errors = {struct_errs}")

    name_ok, name_unchanged, name_deltail_ok, name_failed = apply_names(data["names"])
    print(f"apply_names: {name_ok} applied, {name_unchanged} already correct, "
          f"{name_deltail_ok} applied via SN_DELTAIL retry, {len(name_failed)} failed")
    for ea, name, reason in name_failed[:50]:
        print(f"  FAILED name {ea:#x} {name!r}: {reason}")

    cmt_ok, cmt_failed = apply_comments(data["comments"])
    print(f"apply_comments: {cmt_ok} applied, {len(cmt_failed)} failed")
    for ea, text, reason in cmt_failed[:50]:
        print(f"  FAILED cmt {ea:#x} {text[:40]!r}: {reason}")

    type_ok, type_failed = apply_types(data["types"])
    print(f"apply_types: {type_ok} applied, {len(type_failed)} failed")
    for ea, decl, reason in type_failed:
        print(f"  FAILED type {ea:#x} {decl!r}: {reason}")


main()
