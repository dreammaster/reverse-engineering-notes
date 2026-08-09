"""
IDAPython script -- run inside IDA (Alt-F7) with the Rob Blanc 1 IDB open.

Applies ONLY struct/type definitions that have been verified safe against
this specific 2002 binary (not just copied from the 2011 reference source --
see reversing/notes/struct-layout-drift.md for why that matters). As more
types get verified, add them to SAFE_DECLS below; do NOT add anything here
that hasn't been checked against known IDB struct sizes or allocation-size
evidence in the disassembly.

Currently verified safe:
  - `block` = BITMAP* : a trivial pointer typedef, size-independent of
    BITMAP's actual (currently only partially known) internal layout.
  - `GUIMain`: a PARTIAL struct. Unlike everything else in this file, this
    was NOT copied from the 2011 reference source at all -- every named
    field/size below was read directly off real disassembly instructions
    while matching GUIMain member functions (remove_popup_interface,
    is_mouse_on_gui, mouse_but_down, mouse_but_up). See
    reversing/notes/struct-layout-drift.md for the full derivation of each
    offset. Total size (0x184, confirmed via an `imul reg, 184h` array-
    index computation in remove_popup_interface) is exact; the `objs[60]`
    array length is inferred (0x184 - 0x94 = 0x90 bytes = 60 pointers) from
    that known total size and the known array-start offset, not
    independently confirmed at its far end -- flagged in case a later find
    contradicts it. Unknown regions are left as opaque `_padN` byte arrays,
    not guessed field names.

roomstruct needs no action: the IDB's existing struct already has all 4
fields named and its size (0x10) exactly matches the reference source's
4-pointer layout -- already complete.

  - `CharacterInfo`: UPDATES the IDB's existing struct in place (via
    parse_decls' PT_REPLACE flag -- see main() below; never delete an
    existing type that might already be referenced elsewhere, e.g. by an
    applied function prototype). Started with only field_0, walking, inv
    named; now has 18 fields identified (defview, talkview[tentative],
    view, room, prevroom[tentative], x, y, wait, flags, idletime,
    idleleft, activeinv, loop, frame, walking, animating, walkspeed,
    animspeed[lower-confidence]) recovered directly from disassembly
    evidence (reversing/scripts/find_struct_accesses.py against
    `playerchar`, plus manual reading of update_stuff's, walk_character's,
    and ReleaseCharacterView/Character_UnlockView's per-character logic)
    against the already-matched functions that touch
    `playerchar`/`chi`/`chin`/`chaa`. Total size (0x140) is unchanged --
    still correct. See reversing/notes/struct-layout-drift.md for full
    derivation, including one self-correction (an earlier "wait" guess at
    +0x08 was wrong; it's actually "view"), one upgrade (the pre-existing
    "walking" at +0x3C, briefly flagged as suspicious in an earlier pass,
    turned out to be correct), and a resolved long-standing contradiction
    (the pre-existing "field_0 is a word" annotation was simply wrong --
    it's `defview`, a full int). NOTE ON FIELD ORDER: unlike most of this
    struct, the first four confirmed fields (defview/view/room at
    +0x00/+0x08/+0x0C) landed at exactly the same offsets 2011's source
    has them, which is why the two tentative fields (talkview, prevroom)
    were filled in by positional inference from that adjacency rather than
    independent verification -- flagged as such, not to the same
    confidence standard as the rest.
  - `ccInstance`: a NEW struct (nothing pre-existing in the IDB). 12 fields
    recovered from reading ccCreateInstanceEx and ccCallInstance (both
    already matched) -- flags, globaldata, globaldatasize, code, codesize,
    strings, stringssize, stack, stacksize, registers[6], pc, and
    instanceof_ (trailing underscore since `instanceof` is awkward as a
    bare identifier). Total size (0x9A8, confirmed via the `push 9A8h;
    call malloc` allocation site) is exact and unchanged from what was
    already known. Also resolved a previously-abandoned open lead in the
    process: reading how ccCreateInstanceEx sets instanceof_ and
    increments the source script's own refcount led directly to
    identifying sub_42B054 as ccFreeInstance (see matches.json and
    reversing/notes/struct-layout-drift.md). registers[] itself started as
    an unconfirmed guess but was upgraded once ccCallInstance showed
    registers[1] used exactly like SREG_SP (stack pointer) -- see the
    field comment for why it's 6 longs here, not 2011's 8. One thing still
    flagged rather than asserted: the ~2400-byte gap between stringssize
    and stack is very likely the call-stack bookkeeping arrays but not
    verified field-by-field.

  - `ccScript`: a NEW struct, recovered from reading ccCreateInstanceEx,
    ccCallInstance, and fread_script (all already matched). 13 fields:
    globaldata, globaldatasize, code, codesize, strings, stringssize,
    imports[600], numimports, exports[600], export_addr[600], numexports,
    instances. Total size (0x1C50, confirmed via fread_script's
    `push 1C50h; call malloc`) matches the derived layout exactly --
    `instances` at +0x1C4C plus its own 4 bytes lands precisely on 0x1C50,
    a strong self-consistency check. The three 600-entry arrays
    (imports/exports/export_addr) all derive to exactly the same length
    independently (from three different confirmed offset pairs), which
    looks like a genuine 2002 fixed-capacity limit (later replaced by
    2011's dynamically-sized arrays) rather than coincidence. One
    tentative/positional-only field: a 12-byte gap at +0x18 that matches
    the combined size of 2011's fixuptypes+fixups+numfixups, not
    independently verified.

  - `GUIButton`: a NEW struct (flat -- IDA structs don't model C++
    inheritance, so GUIObject's base fields are inlined at their absolute
    offsets). Recovered by reading the actual GUIButton vtable directly
    out of `.rdata` (starting at 0x4AD4A0) alongside three newly-matched
    vtable methods (MouseDown, MouseUp, ReadFromFile) plus the
    already-matched Draw. The vtable slot order matches 2011's declared
    virtual-method order with zero drift (MouseMove, MouseOver,
    MouseLeave, MouseDown@+0xC, MouseUp@+0x10, KeyPress, Draw@+0x18, ...) --
    see reversing/notes/struct-layout-drift.md for the full slot mapping.
    Confirmed fields: flags@+0x04 (GUIObject base), activated@+0x1C
    (GUIObject base), text[50]@+0x20, and a 12-int/48-byte block from
    +0x54 to +0x80 (pic, overpic, pushedpic, usepic, ispushed, isover,
    font, textcol, leftclick, rightclick, lclickdata, rclickdata) that
    matches 2011's declared order exactly with zero drift. NOTE: this is
    a minimum/partial size (0x84), not a confirmed total -- 2011 has more
    fields after rclickdata not yet confirmed for this build.

Deliberately NOT included (confirmed drifted, no field-level recovery done
yet, see notes): SpriteCache, GameSetupStructBase. Applying their 2011
layouts would silently inject wrong field offsets into the IDB.
"""
try:
    import idc
    IN_IDA = True
except ImportError:
    IN_IDA = False


SAFE_DECLS = r"""
typedef BITMAP *block;

struct GUIMain {
  char _pad0[0x28];       // unknown -- not yet touched by any matched function
  int x;                  // +0x28, confirmed via GUIMain::mouse_but_down
  int y;                  // +0x2C, confirmed via GUIMain::mouse_but_down
  char _pad1[0x24];       // +0x30..0x54, unknown
  int mouseover;          // +0x54, confirmed via GUIMain::mouse_but_down
  char _pad2[0x08];       // +0x58..0x60, unknown
  int mousedownon;        // +0x60, confirmed via GUIMain::mouse_but_up/down
  char _pad3[0x2C];       // +0x64..0x90, unknown
  int on;                 // +0x90, confirmed via remove_popup_interface + is_mouse_on_gui
  void *objs[60];         // +0x94, confirmed start offset; length inferred from total size (see note above).
                           // Elements point to GUIObject-derived instances; virtual method table has
                           // MouseDown at +0xC and MouseUp at +0x10 (not modeled here -- IDA structs don't hold vtables).
};

struct CharacterInfo {
  int defview;              // +0x00, high confidence, RESOLVES the earlier "field_0 is a word" mystery:
                           // Character_UnlockView (Engine/acchars.cpp:1173, what ReleaseCharacterView
                           // delegates to) does "chaa->view = chaa->defview;" -- disasm matches exactly,
                           // [+0x08](view) = [+0x00], and it's read as a full 32-bit int, not a word,
                           // contradicting the old pre-existing "word" annotation which is now discarded.
  int talkview;              // +0x04, TENTATIVE, positional inference only (not independently verified):
                           // defview/view/room landed at +0x00/+0x08/+0x0C, exactly matching the 2011
                           // source's field order/spacing (defview,talkview,view,room,prevroom all
                           // 4-byte ints in that order) -- talkview is the 2011 field that would fall
                           // at +0x04 if that adjacency holds. Treat as a lead, not a confirmed fact.
  int view;                // +0x08, high confidence, SUPERSEDES an earlier "wait" guess (see
                           // reversing/notes/struct-layout-drift.md for the retraction and why).
                           // update_stuff uses this as `imul ecx, 8D4h` array index into a per-view
                           // data table -- an unambiguous "this is a view number" pattern. Also directly
                           // confirmed again via Character_UnlockView's defview assignment (see above).
  int room;                // +0x0C, high confidence: SetPlayerCharacter saves this, switches
                           // playerchar, and calls NewRoom(new playerchar->room) if it changed --
                           // matches source exactly.
  int prevroom;              // +0x10, TENTATIVE, positional inference only -- see talkview above; this
                           // is the 2011 field that would fall here if the defview..room adjacency holds.
  int x;                   // +0x14, high confidence: exact arg-order match in get_hotspot_at's
                           // caller (mainloop), matching source's get_hotspot_at(playerchar->x, playerchar->y).
  int y;                   // +0x18, see x above.
  int wait;                 // +0x1C, high confidence, RESOLVES the round-2 "decrement-if-positive
                           // countdown at +0x1C" lead (which round-2 mis-attributed to +0x08, since
                           // corrected to "view"): Character_LockView (Engine/acchars.cpp:824, what
                           // SetCharacterView delegates to) does "chap->wait=0;" -- matches [+0x1C]=0
                           // exactly. update_stuff's "if (chi->wait>0) chi->wait--;" lip-sync decrement
                           // pattern (originally spotted in round 2) belongs here, not at +0x08.
  int flags;                // +0x20, high confidence: Character_UnlockView does
                           // "chaa->flags &= ~CHF_FIXVIEW;" where CHF_FIXVIEW=2 (Common/acroom.h:2480) --
                           // disasm matches exactly: `and al, 0FDh` (~2) on a 32-bit field read via `mov eax,[...]`.
  char _pad2b[0x08];      // +0x24..0x2C, unknown
  short idletime;           // +0x2C, high confidence: walk_character does
                           // "chin->idleleft=chin->idletime;" -- disasm matches verbatim:
                           // [+0x2E] = [+0x2C] inside the exact "if (chin->idleleft < 0)" branch.
  short idleleft;           // +0x2E, high confidence: same evidence as idletime -- walk_character's
                           // "if (chin->idleleft < 0) { ReleaseCharacterView(...); ... }" matches a
                           // signed [+0x2E]<0 check driving a call to the already-matched ReleaseCharacterView.
                           // Also independently re-confirmed via Character_UnlockView's own
                           // "chaa->idleleft = chaa->idletime;" at its end.
  char _pad2c[0x04];      // +0x30..0x34, unknown
  int activeinv;            // +0x34, high confidence: SetActiveInventory(-1) sets this to -1,
                           // matching source's "player.activeinv = -1;" exactly.
  short loop;               // +0x38, medium confidence: update_stuff writes an 8-entry direction-lookup
                           // table value here (source: Engine/acchars.cpp:19 "int turnlooporder[8] =
                           // {0, 6, 1, 7, 3, 5, 2, 4};" -- the classic AGS "walking direction -> loop
                           // number" table, confirmed to be the same table via ReleaseCharacterView's context).
  short frame;              // +0x3A, high confidence: Character_UnlockView's "chaa->frame = 0;" matches
                           // disasm exactly ([+0x3A] = 0, right where source resets frame after defview restore).
  short walking;           // +0x3C, pre-existing IDB annotation, CONFIRMED (upgraded from "flagged" in an
                           // earlier pass): walk_character assigns find_route's result directly into this
                           // field ("chin->walking = <route data>", roughly) right after computing the
                           // path -- explains the modular-1000/10000 arithmetic seen elsewhere too (it's
                           // processing a packed route/movelist-derived value, not a plain boolean).
  short animating;          // +0x3E, high confidence: walk_character's
                           // "if (chin->animating && autoWalkAnims) chin->animating = 0;" matches
                           // disasm exactly (test-and-clear-to-0 on this field, gated the same way).
  short walkspeed;          // +0x40, high confidence: walk_character reads this into a global right where
                           // source has "int move_speed_x = chin->walkspeed;" (the very next source line
                           // after the animating check above).
  short animspeed;          // +0x42, tentative/lower confidence: inferred from positional adjacency to
                           // walkspeed (matches 2011's "short walkspeed, animspeed;" pair) plus an earlier,
                           // less certain read-site seen during unrelated work -- not independently nailed
                           // down to the same standard as the fields above.
  int inv;                 // +0x44, pre-existing IDB annotation, kept as-is. CAUTION: named "inv" by
                           // earlier work, presumably after 2011's short inv[MAX_INV] array -- but this
                           // 2002 struct is far too small (0x140 total) to hold a 301-element array, so
                           // if a per-character inventory array exists here at all it must be a very
                           // different size/shape. Not re-verified this pass; flagged for a closer look.
  char _pad5[0xF8];        // +0x48..0x140, unknown (248 bytes, pre-existing "field_48")
};

struct ccScript;  // forward declaration -- full definition follows ccInstance below, since
                  // ccInstance only needs a pointer to it (same forward-then-define pattern
                  // the 2011 source itself uses, Common/CSCOMP.H:21-22).

struct ccInstance {
  int flags;                 // +0x00, high confidence: ccCreateInstanceEx sets this to 0, or 1
                            // (matching INSTF_SHAREDATA, Common/CSCOMP.H:235) when joining an existing
                            // instance's global data -- matches source's flags/INSTF_SHAREDATA role exactly.
  void *globaldata;          // +0x04, high confidence: mallocated (when not sharing) and memcpy'd from
                            // the source ccScript's own +0x00 field, matching source's
                            // "instance->globaldata = malloc(...); memcpy(..., scri->globaldata, ...)".
  int globaldatasize;         // +0x08, high confidence: the size driving the above malloc/memcpy, copied
                            // from the source ccScript's +0x04 field (ccScript::globaldatasize).
  unsigned long *code;         // +0x0C, high confidence: same malloc/memcpy pattern as globaldata, size
                            // scaled by `shl reg,2` (sizeof(long)) matching "array of longs".
  int codesize;                // +0x10, high confidence: the (pre-scaling) count driving the above,
                            // copied from the source ccScript's +0x0C field.
  char *strings;               // +0x14, high confidence: NOT malloc'd/copied -- pointer copied directly
                            // from the source ccScript's +0x10 field (shared, read-only script string pool).
  int stringssize;             // +0x18, high confidence: copied directly from ccScript's +0x14 field,
                            // alongside strings above (same non-owning-copy treatment).
  char _pad_calls[0x960];    // +0x1C..0x97C, unknown (2400 bytes). Almost certainly holds the call-stack
                            // bookkeeping arrays (2011 has three parallel MAX_CALL_STACK=100 arrays here --
                            // callStackLineNumber/Addr/CodeInst -- but the exact 2002 composition, including
                            // whether MAX_CALL_STACK is still 100, has not been independently verified).
  char *stack;                // +0x97C, high confidence: malloc'd right where source computes register/stack
                            // setup, using the size stored at the very next field.
  int stacksize;               // +0x980, high confidence: the size used for the above malloc. Value
                            // observed at this call site is 0x7D0 (2000) -- NOTE this differs from the
                            // CC_STACK_SIZE=4000 macro in the 2011 source (Common/CSCOMP.H:239); either the
                            // macro's value was smaller in 2002, or it's measured in different units here
                            // (e.g. longs vs bytes) -- not resolved, flagged for a closer look.
  int registers[6];           // +0x984, high confidence (upgraded from padding): ccCallInstance uses
                            // [Block+0x988] exactly like a stack-pointer register -- initialized from
                            // `stack`, incremented by (argcount*4) to push args, decremented back after
                            // the interpreter returns. 0x988 is registers[1] if the array starts at
                            // 0x984 (4-byte stride), and SREG_SP=1 in the 2011 source (Common/CSCOMP.H:227)
                            // -- exact match. 2011 has CC_NUM_REGISTERS=8 (32 bytes: SREG_SP=1, MAR=2,
                            // AX=3, BX=4, CX=5, OP=6, DX=7), but this region is only 24 bytes -- fits 6
                            // longs (indices 0-5), not 8. SREG_OP ("object pointer for member func
                            // calls") and SREG_DX are index 6/7, past the end of this 2002 region --
                            // plausible that those two registers didn't exist yet in 2002, consistent
                            // with member-function-call script features being a later addition. Only
                            // registers[1]/SP independently confirmed; the other 5 slots are inferred
                            // from the array's existence, not individually verified.
  int pc;                      // +0x99C, high confidence: set to 0 right after the registers-sized region,
                            // matching source's program-counter reset on instance creation.
  char _pad_9A0[0x04];       // +0x9A0..0x9A4, unknown
  struct ccScript *instanceof_; // +0x9A4, high confidence (named instanceof_ -- `instanceof` is a C++
                            // reserved word in some contexts / awkward as a plain identifier, kept
                            // trailing underscore to be safe for IDA's parser): set to the source
                            // ccScript pointer at creation, and is exactly the field ccFreeInstance
                            // (sub_42B054, matched via this same investigation -- see matches.json)
                            // reads to reach back to the originating script and decrement its refcount.
};

struct ccScript {
  char *globaldata;            // +0x00, high confidence: ccCreateInstanceEx mallocs/memcpy's a new
                            // instance's own globaldata from here (see ccInstance above).
  int globaldatasize;          // +0x04, high confidence: the size driving the above copy.
  unsigned long *code;         // +0x08, high confidence: same pattern as globaldata -- source for the
                            // instance's own malloc'd/memcpy'd code array.
  int codesize;                 // +0x0C, high confidence: the (pre-scaling) count driving the above.
  char *strings;                // +0x10, high confidence: pointer an instance copies directly (not
                            // malloc'd) into its own `strings` field -- shared read-only string pool.
  int stringssize;              // +0x14, high confidence: copied directly alongside strings.
  char _pad_18[0x0C];         // +0x18..0x24, unknown (12 bytes). TENTATIVE positional-only inference:
                            // matches the exact combined size of 2011's fixuptypes(4)+fixups(4)+
                            // numfixups(4), which would fall here if that adjacency holds -- not
                            // independently verified, same caution as CharacterInfo's talkview/prevroom.
  void *imports[600];          // +0x24, high confidence: ccCreateInstanceEx null-checks entries in this
                            // array (`[arg_0+ecx*4+24h]`) in a loop bounded by numimports below. Length
                            // (600) is exact: (numimports's offset 0x984 - 0x24) / 4 = 600, and this
                            // figure is independently corroborated by exports[]/export_addr[] below
                            // being exactly the same length -- looks like a real 2002 MAX_IMPORTS/
                            // MAX_EXPORTS=600 fixed-array limit (later replaced by 2011's dynamic
                            // importsCapacity-sized allocation).
  int numimports;               // +0x984, high confidence: confirmed loop bound for the imports[] scan above.
  char *exports[600];           // +0x988, high confidence: ccCallInstance strcmp's function names against
                            // entries here (`[instanceof+ecx*4+988h]`) in a loop bounded by numexports
                            // below. Same 600-entry length as imports[], see note above.
  long export_addr[600];        // +0x12E8, high confidence: ccCallInstance reads a packed value here
                            // (`[instanceof+ecx*4+12E8h]`), high byte masked/shifted out as a type tag
                            // (must equal 1 for "is a function") -- matches 2011's comment on this field
                            // exactly ("high byte is type; low 24-bits are offset").
  int numexports;               // +0x1C48, high confidence: confirmed loop bound for exports[]/export_addr[]
                            // above; also sits directly adjacent to instances below, same as 2011's order.
  int instances;                 // +0x1C4C, high confidence: incremented by ccCreateInstanceEx when an
                            // instance is created (`instanceof->instances++`), decremented and checked
                            // against 0 by ccFreeInstance -- this is what originally led to identifying
                            // ccFreeInstance (see reversing/notes/struct-layout-drift.md). Also confirmed
                            // set to 0 by fread_script right after allocation.
};

struct GUIButton {
  // Flat layout (IDA structs don't model C++ inheritance) -- the first few fields below are
  // really GUIObject's (the base class), confirmed via GUIButton's own methods reading them
  // through `this`. Recovered by reading the actual GUIButton vtable directly out of .rdata
  // (starting at address 0x4AD4A0) plus three already-matched vtable methods (MouseDown,
  // MouseUp, ReadFromFile). See reversing/notes/struct-layout-drift.md for the full vtable
  // slot mapping (matches 2011's declared virtual-method order with zero drift, slots 0-8).
  void *vtbl;                  // +0x00, implicit (not read directly, inferred from calling convention)
  unsigned int flags;           // +0x04, high confidence (GUIObject base field): confirmed via
                            // GUIButton::MouseUp's IsDisabled()/IsClickable() check (`[this+4] & 4`).
  char _pad_unknown[0x14];    // +0x08..0x1C, unknown (GUIObject base fields -- 2011 has guin/objn/x/y/
                            // wid/hit here, but exact 2002 sub-offsets not individually confirmed).
  int activated;                // +0x1C, high confidence (GUIObject base field): confirmed via
                            // GUIButton::MouseUp's "activated++" and via GUIObject::ReadFromFile's
                            // 7-int (28-byte) base-field block ending exactly here.
  char text[50];                // +0x20, high confidence: confirmed via GUIButton::ReadFromFile's
                            // "fread(&text[0], sizeof(char), 50, ooo)" landing exactly at [this+0x20],
                            // immediately after GUIObject's persisted base fields end at +0x20.
  char _pad_align[2];         // +0x52..0x54, alignment padding (50 isn't 4-byte aligned)
  int pic;                      // +0x54, high confidence
  int overpic;                  // +0x58, high confidence (confirmed via MouseUp: usepic=overpic)
  int pushedpic;                 // +0x5C, high confidence (confirmed via MouseDown: usepic=pushedpic)
  int usepic;                    // +0x60, high confidence (confirmed via MouseDown and MouseUp)
  int ispushed;                  // +0x64, high confidence (confirmed via MouseDown/MouseUp setting it 1/0)
  int isover;                    // +0x68, high confidence (confirmed via MouseUp's isover check)
  int font;                      // +0x6C, high confidence (positional, within the confirmed pic..rclickdata
                            // 12-int/48-byte block read by ReadFromFile -- matches 2011's declared order
                            // exactly for this whole block, zero drift, see notes)
  int textcol;                   // +0x70, high confidence: confirmed directly via ReadFromFile's
                            // "if (textcol==0) textcol=16;" matching [this+0x70]==0 exactly
  int leftclick;                 // +0x74, high confidence (positional, see font above)
  int rightclick;                // +0x78, high confidence (positional, see font above)
  int lclickdata;                // +0x7C, high confidence (positional, see font above)
  int rclickdata;                // +0x80, high confidence (positional, see font above)
  // NOTE: this is a MINIMUM/partial size, not a confirmed total. 2011 has more fields after
  // rclickdata (textAlignment, reserved1, eventHandlers[]...) that are read/written individually
  // (not via the three bulk fread/fwrite calls this recovery was based on) -- not yet confirmed
  // for this 2002 build. Do not assume sizeof(GUIButton) == 0x84.
};
"""


def main():
    if not IN_IDA:
        print("This script must be run inside IDA (idc not importable).")
        return

    # CharacterInfo already existed in the IDB's Local Types library (it had a
    # "mappedto_85" ordinal, meaning other type info -- e.g. load_new_room's
    # already-applied CharacterInfo* parameter -- references it by that
    # ordinal, not just by name). An earlier version of this script deleted it
    # via ida_struct.del_struc() before recreating it, which broke that
    # ordinal link (existing references started showing as the orphaned "#85"
    # instead of "CharacterInfo") without properly cleaning it up.
    #
    # PT_REPLACE tells parse_decls to update an existing same-named type's
    # contents IN PLACE, preserving its ordinal/identity, so anything already
    # pointing at it (like that parameter) stays correctly linked. This is
    # the correct way to redefine an existing type -- never del_struc() a type
    # that might already be referenced elsewhere.
    err_count = idc.parse_decls(SAFE_DECLS, idc.PT_SILENT | idc.PT_REPLACE)
    if err_count == 0:
        print("OK: applied verified-safe type declarations (block, GUIMain, CharacterInfo).")
    else:
        print(f"parse_decls reported {err_count} error(s) -- check the declarations above.")


if __name__ == "__main__":
    main()
