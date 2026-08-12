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
    is_mouse_on_gui, mouse_but_down, mouse_but_up, get_control_type). See
    reversing/notes/struct-layout-drift.md for the full derivation of each
    offset. Total size (0x184, confirmed via an `imul reg, 184h` array-
    index computation in remove_popup_interface) is exact. `objs[30]` and
    `objrefptr[30]` (added via get_control_type) are confirmed as separate
    parallel arrays filling +0x94..+0x184 exactly, matching 2011's
    MAX_OBJS_ON_GUI=30 with zero drift -- this CORRECTS an earlier
    `objs[60]` guess made before objrefptr[] was known to exist (that guess
    inferred length purely from total-size arithmetic, which silently
    assumed objs[] was the only thing filling that space). Unknown regions
    are left as opaque `_padN` byte arrays,
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

  - `GUISlider`: a NEW struct, recovered by finally locating its vtable at
    off_4AD530 -- a table sitting BETWEEN the already-pinned GUIInv
    (off_4AD50C) and GUITextBox (off_4AD554) tables that was initially
    skipped over entirely during that sweep (see reversing/notes/
    struct-layout-drift.md for how off_4AD578 was briefly and incorrectly
    suspected to be GUISlider before being correctly reassigned to
    GUIListBox). `mpressed@+0x2C` is confirmed three independent ways: set
    to 1 by MouseDown, cleared to 0 by MouseUp, and guarded on
    ("if (mpressed==0) return;") at the top of MouseMove before its
    floating-point drag-ratio computation. WriteToFile's 16-byte bulk
    fwrite at +0x20 exactly covers min/max/value/mpressed (the first 4 ints
    declared at Common/acgui.h:216-217), landing mpressed at +0x2C in
    total agreement with the other three methods. DRIFT: the 3 fields
    declared right after (handlepic, handleoffset, bgimage) are absent
    from this build's bulk WriteToFile.

  - `GUIListBox` / `GUIInv`: two more NEW structs, recovered the same way --
    pinning off_4AD578 (GUIListBox) and off_4AD50C (GUIInv) via DATA XREFs on
    methods whose bodies are unambiguous. `GUIListBox::MouseMove`'s
    "mousexp=nx-x; mouseyp=ny-y;" (acgui.h:408-409) was initially mistaken
    for GUISlider based on vtable-slot shape alone (non-empty MouseMove/
    MouseDown, empty KeyPress looked plausible for a draggable control) --
    reading the actual body (no `mpressed` guard, no float math) ruled that
    out and the mousexp/mouseyp assignment pattern is unique to GUIListBox.
    Its WriteToFile's 44-byte bulk fwrite at +0x1B0 exactly reproduces the
    11-int field block declared at acgui.h:388-390 (numItems..exflags), and
    cross-checks perfectly against mousexp@+0x1BC/mouseyp@+0x1C0 landing at
    the expected 4th/5th position -- strong self-consistent evidence. DRIFT:
    the 3 fields declared after exflags (selectedbgcol, alignment, reserved1)
    are absent from this build's WriteToFile. GUIInv was pinned via
    MouseOver/MouseLeave/MouseUp all touching a single field (`isover`) at
    +0x20 (GUIInv's own first field, right after the GUIObject base --
    same pattern as GUIButton::text/GUITextBox::text/GUIListBox::items[]).
    DRIFT: GUIInv's WriteToFile/ReadFromFile in this build do ONLY the base
    28-byte block -- no putw/getw for charId/itemWidth/itemHeight/topIndex
    at all, consistent with 2011's own version-gated fallback path
    ("if (version>=109) {...} else {charId=-1; ...}", acgui.h:486-497) --
    this build predates that format version, so those 4 fields are not
    (yet, or ever, for this build) part of GUIInv's persisted layout;
    Draw() for both classes accepted at medium confidence (positional +
    plausible shape) since their bodies weren't traced statement-by-statement.
    `items[]`/`saveGameIndex[]` (GUIListBox's own two arrays, filling
    +0x20..+0x1B0, 0x190 bytes total) are NOT individually split out here --
    the arithmetic doesn't divide evenly assuming naive 4-byte pointer +
    2-byte short entries at equal MAX_LISTBOX_ITEMS length (6*N=0x190 isn't
    an integer N), so this region is left as opaque padding pending real
    per-field evidence rather than a guessed split.

  - `GUITextBox` / `GUILabel`: two NEW structs, recovered together by pinning
    two adjacent 9-slot vtables directly in .rdata (off_4AD554 and off_4AD4E8)
    via DATA XREF addresses on their own already-matched methods (e.g.
    GUITextBox__KeyPress's "DATA XREF: .rdata:004AD568" lands exactly at
    off_4AD554+0x14, vtable slot 5). Both share a fixed `char text[200]`
    inline array at +0x20 followed by a 3-int tail at +0xE8 (font/textcol/
    +one more int), recovered via matching fwrite/fread ElementSize=0xC8(200)
    and ElementSize=3,ElementCount=4 calls in their WriteToFile/ReadFromFile.
    GUILabel's fixed-array text[200] is notable: the 2011 source has since
    replaced it with a dynamically-allocated `char *text` + `textBufferLen`,
    but the *old* fixed-200-byte fwrite is still present as a dead commented-
    out line right in the 2011 source (acgui.cpp:276, "//fwrite(&text[0],
    sizeof(char), 200, ooo);"), and 2011's own ReadFromFile explicitly branches
    "if (version<113) textBufferLen=200;" -- both are direct textual
    confirmation that this 2002 build predates that refactor. GUITextBox's
    own text[200]/font/textcol/exflags order matches 2011 with zero drift.
    IMPORTANT GENERALIZABLE FINDING from this round: GUITextBox::Draw's
    wrectangle(x,y,x+wid-1,y+hit-1) call resolves the GUIObject base class's
    previously-unconfirmed x/y/wid/hit fields precisely: x@+0x08, y@+0x0C,
    wid@+0x10 (independently reconfirmed via GUITextBox::KeyPress's text-width
    bound check), hit@+0x14 -- this CORRECTS GUIButton's struct below, whose
    +0x08..+0x1C span was previously left as opaque padding.

  - `SpriteCache`: a struct that turns out to be ALREADY fully recovered
    directly in the live IDB (predating this project's apply_structs.py
    tracking, part of the original pre-existing manual work) -- only
    formalized into this script now for re-runnability. Just 4 fields
    (`offsets`, `elements`, `images`, `ff`) filling the known IDB size
    exactly (0x10 = 16 bytes), confirmed via SpriteCache::initFile
    (Common/sprcache.cpp:631-643, already matched): "for (vv=0; vv<elements;
    vv++) { images[vv]=NULL; offsets[vv]=0; } ff=clibfopen(filnam,\"rb\");"
    matches the disasm's loop-bound/array-write/fopen-assignment pattern
    exactly. DRASTIC DRIFT from 2011: `class SpriteCache` there has ~16
    data members (LRU eviction tracking: mrulist/mrubacklink/liststart/
    listend/lastLoad/maxCacheSize/lockedSize; per-sprite metadata:
    sizes/flags/spritesAreCompressed; cachesize accounting) -- none of
    that exists in this 2002 struct at all. The whole cache-eviction
    subsystem is a later addition; 2002's SpriteCache is just two raw
    parallel arrays (offsets/images) plus a count and a file handle, with
    no size limit or discard policy. Since the struct is fully accounted
    for byte-for-byte, no further field-level work is expected here.

  - `GameSetupStructBase`: a PARTIAL struct -- 34 fields confirmed so far
    (`gamename`, `options`, `paluses`, `defpal[256]`, `iface[10]`,
    `numiface`, `numviews`, `mcurs[10]`, `globalscript`,
    `numcharacters`, `chars`, `__charcond[50]`, `__invcond[100]`,
    `compiled_script`, `playercharacter`, `totalscore`, `numinvitems`,
    `numdialog`, `numdlgmessage`, `numfonts`, `color_depth`,
    `target_win`, `dialog_bullet`, `hotdot`, `hotdotouter`, `uniqueid`,
    `reserved[2]`, `numlang`, `langcodes[5][3]`, `messages[500]`,
    `fontflags[10]`, `fontoutline[10]`, `numgui`, `dict`) -- EVERY field
    `OriGameSetupStruct`/`OriGameSetupStruct2` declares is now
    accounted for, out of a known total size of 0xBF84 (49028 bytes).
    `messages[500]` was confirmed via load_ac2game_dta (already
    matched): a per-slot conditional malloc+fread loop, immediately
    followed by the already-confirmed `set_default_glmsg` chain for the
    12 built-in messages -- landing EXACTLY where `reserved[2]`+
    `numlang`+`langcodes[5][3]`+alignment predict, which in turn is
    EXACTLY `0x800` (2048) bytes before the already-confirmed `numgui`
    once `fontflags[10]`/`fontoutline[10]` are added -- an
    over-determined fit (two confirmed endpoints AND an independently-
    confirmed anchor in the middle all agreeing exactly), stronger than
    a typical two-endpoint arithmetic argument. `reserved[2]`/
    `numlang`/`langcodes`/`fontflags`/`fontoutline` themselves have no
    access-site evidence of their own (2011's own source has zero live
    references to most of them -- `numlang`/`langcodes`' only 2011
    mention is itself commented-out dead code, and `fontflags`/
    `fontoutline`'s 2011 reader lives in a structurally divergent,
    much-later loading routine with no counterpart in this build) --
    included at medium confidence, boxed in by the exact-fit arithmetic
    rather than independently confirmed field-by-field.
    `defpal[256]` and `iface[10]` -- the LAST major
    unrecovered span in this struct -- were fully resolved this round:
    `defpal` was previously RETRACTED (see the retraction note that
    used to live here) based only on IDA's declared label extent, never
    on whether the actual code reads further -- it does, via a second,
    independent copy of the "for(ee=0;ee<256;ee++) if(paluses[ee]!=
    PAL_BACKGROUND) palette[ee]=defpal[ee];" loop inside `main`, reading
    a full 1024 bytes (4-byte stride, 256 entries) starting right after
    `paluses`. `iface[10]` was a documented-but-unasserted arithmetic-
    fit hypothesis before this round found real field-level evidence:
    `byte_513B7C`/`byte_513B7D` (previously assumed to belong to some
    unrelated "legacy interface" global, `g_interface`) sit at EXACTLY
    the byte offsets `InterfaceElement.popup`/`.on` would predict within
    this array. Both findings together also explain what `g_interface`
    actually was all along: not a separate global, just IDA's own
    mis-labeled sub-range of `defpal`+`iface[10]`'s own memory (its
    address falls INSIDE the now-confirmed 1024-byte `defpal` span).
    `__charcond`/`__invcond` were promoted from an arithmetic-fit
    hypothesis to fully confirmed `EventBlock` arrays in an earlier
    round -- see the new `EventBlock` struct below, confirmed
    field-by-field via `run_event_block` (`sub_417088`). `mcurs[10]`
    was ALSO confirmed in an earlier round: its own struct
    (`MouseCursor`, moved up in this file to be defined before
    `GameSetupStructBase` since it's now embedded directly, not just
    referenced by pointer) was already fully recovered several rounds
    ago -- that round just traced the array's base address back to its
    exact position inside `GameSetupStructBase` for the first time.
    Zero open field-identity leads remain in this struct: every
    candidate field is now either confirmed present or confirmed absent
    (`numcursors`, `default_lipsync_frame`, `invhotdotsprite`,
    `default_resolution`).
    This is a much bigger undertaking than any other struct in this
    project (2011's GameSetupStructBase is ~3900 bytes; the derived
    GameSetupStruct with its fixed-size arrays gets closer, but the true
    2002 layout is unknown beyond what's below).
    MAJOR FINDING: this global's true identity is `OriGameSetupStruct`
    (`Common/acroom.h:2769`) -- AGS's own OLDEST ancestor struct in its
    save-compatibility evolution chain (`OriGameSetupStruct` ->
    `OriGameSetupStruct2` -> `OldGameSetupStruct` -> ... ->
    `GameSetupStructBase`), preserved read-only in the 2011 header via
    `ConvertOldGameStruct` (`acroom.h:3017`) purely for old-save
    upgrading -- see struct-layout-drift.md for the full writeup. This
    retroactively explains nearly every "drastic drift" finding below
    (byte-sized `options[20]`, `gamename[30]`, the field-order
    divergence) as one fact, not scattered coincidences.
    Recovered via the global instance `game_gamename` (confirmed as the
    struct's base address, i.e. field_0, via load_ac2game_dta's
    `fread(&game_gamename, sizeof GameSetupStructBase, 1, file)` -- a
    single bulk read of the whole struct, typical for a little-endian
    platform, so it does NOT reveal individual field boundaries by itself)
    plus already-matched functions' incidental accesses to individual
    bytes/dwords within the blob.
    IMPORTANT METHODOLOGY NOTE, learned the hard way (twice): this .data
    region already had numerous individual byte/dword labels (e.g.
    `byte_513337`, `dword_51344A`) from IDA's own heuristics/earlier
    work, each with real DATA XREFs -- but their DECLARED SIZES ("db ?",
    "dd 33h dup(?)", etc.) are NOT reliable field-boundary evidence by
    themselves: IDA only splits off a named label as far as it sees a
    DIRECT reference land, so a label showing 51 dwords does not prove
    the true field extends no further. Nor is a LOOP BOUND alone
    sufficient in the other direction -- a first pass here wrongly
    asserted a 256-element `defpal` field purely from a matching
    `for(ee=0;ee<256;...)` loop in Engine/AC.CPP, without checking that
    256 dwords of adjacent .data space actually existed. Mechanically
    re-counting raw bytes found only ~51 dwords before hitting a totally
    unrelated global (a script-exported legacy object, just placed
    memory-adjacent by the linker) -- so that field was RETRACTED rather
    than shipped wrong. Verify a field's size against BOTH an access
    pattern AND independently-confirmed adjacent memory (or a real
    allocation-size constant) -- neither one alone is enough. (`defpal`
    was LATER properly reconfirmed, in a much later round, once a
    SECOND independent access site -- reading the full 1024 bytes, not
    just the loop bound -- was found; see `GameSetupStructBase`'s own
    field comment below. The "unrelated global" it collided with,
    `g_interface`, turned out to not be a separate global at all -- see
    struct-layout-drift.md's later resolution. This retraction's own
    LESSON still stands even though the specific verdict on `defpal`
    itself later flipped -- the methodology was right, the available
    evidence at the time just wasn't enough yet.) THE SECOND
    LESSON (this round): a mechanical byte-offset-counting SCRIPT is
    also not automatically trustworthy -- `align N` directives align the
    TRUE ABSOLUTE memory address, not a running offset relative to some
    arbitrary base, and `game_gamename`'s own base (`0x513318`) is not
    16-byte aligned, so a script that rounds the relative offset instead
    of the true absolute address silently drifts by a few bytes at every
    `align` boundary. Caught by cross-checking the script's output
    against DIRECT hex-address subtraction (using labels whose IDA name
    embeds their own real address, like `dword_51594C`) for several
    fields before trusting any of them -- always do this cross-check
    when a script computes an offset, the same discipline as the
    access-pattern-vs-adjacent-memory rule above. See
    struct-layout-drift.md for both writeups in full.

  - `MouseCursor` (game's `mcurs[]` array, `Common/acroom.h:2455`): a NEW
    struct, found incidentally while investigating GameSetupStructBase's
    `hotdot`/`hotdotouter` fields. 5 of 6 fields independently confirmed
    with real evidence, matching 2011's declared order with ZERO drift --
    the strongest confirmation-to-effort ratio of any struct in this
    project. Stride confirmed at exactly `0x18` (24 bytes) via a
    consistent `imul reg,18h` array-index pattern across 4+ already-
    matched functions (ChangeCursorGraphic, SetMouseCursor,
    __GetLocationType, a cursor-precache loop in `main`). `pic`(int,+0),
    `hotx`(short,+4), `hoty`(short,+6), `view`(short,+8) confirmed via
    `dword_51585C`/`word_515860`/`word_515862`/`word_515864` respectively
    -- each address delta between them matches its predicted field size
    exactly (4,2,2 bytes). `flags`(char,+20) confirmed via `byte_515870`,
    checked with `&2`/`&4` bitmasks matching 2011's `MCF_DISABLED=2`/
    `MCF_STANDARD=4` (`acroom.h:2451-2452`) exactly. `name[10]`(+10) is
    the ONLY unconfirmed field -- zero references anywhere in the
    disassembly (plausible for an editor-only descriptive label the
    runtime never reads, same pattern as GameSetupStructBase's
    `target_win`), included at medium confidence since it's boxed in
    with zero slack between the confirmed `view` and `flags` fields.

  - `ExecutingScript` (the `scripts[]` call-stack array, `Common/
    acruntim.h:700`): FULLY mapped, zero unaccounted bytes, `0x00`..`0x6C`
    (108 bytes total). Found via post_script_cleanup (already matched): a
    `rep movsd` bulk-copies exactly `0x1B` dwords (108 bytes) from
    `dword_4CC848[(num_scripts-1)*0x6C]` into a local stack buffer,
    confirming the element stride at exactly `0x6C` (108 bytes) two ways
    (the copy count AND the `imul` indexing used throughout).
    `inst`(ccInstance*,+0x00) is read with NO added offset and passed
    straight to the already-matched `ccFreeInstance`, matching 2011's
    FIRST declared field exactly. `forked`(char,+0x68) gates that same
    `ccFreeInstance` call, matching 2011's LAST declared field exactly
    (`acruntim.h:711`). The middle section was cracked open by decoding
    IDA's OWN pre-existing local-variable names for the stack buffer
    (`newnum`, `ooo`, `dlgnum` at `-0x68`/`-0x60`/`-0x5C` -- computing
    `buffer_offset = local_offset - (-0x6C)` maps each straight onto the
    struct) and confirming each against its actual usage:
    `newnum`(int,+0x04, sentinel -1) feeds `new_room()` -- matches 2011's
    `ePSANewRoom`. An unnamed bool(+0x08) gates a call to `sub_41FEA9`,
    which calls the already-matched `__actual_invscreen()` -- matches
    `ePSAInvScreen`. `ooo`(int,+0x0C) is compared against sentinel `0x3E8`
    (1000, meaning "show `RestoreGameDialog`") else, if `>=0`, passed
    directly to `restore_game_data()` as a save-slot number -- matches
    2011's COMBINED `ePSARestoreGame`/`ePSARestoreGameDialog` pair folded
    into one field plus a magic sentinel (2011 keeps them as two separate
    enum cases). `dlgnum`(int,+0x10, sentinel -1) feeds
    `do_conversation()` -- matches `ePSARunDialog`. Then the
    already-confirmed `run_another` chain: `script_run_another[2][30]`
    (+0x14), `run_another_p1[2]`/`run_another_p2[2]` (+0x50/+0x58),
    `numanother` (+0x60) -- see `ExecutingScript::run_another`
    (`sub_425500`) in matches.json; DRIFT: capacity is 2 here, not 2011's
    declared `MAX_QUEUED_SCRIPTS=4`. Finally an unnamed bool(+0x64,
    between `numanother` and `forked`) gates a call to `RestartGame()` --
    matches `ePSARestartGame`. Cross-confirmed end to end by
    `ExecutingScript::init()` (`sub_424A00`), a constructor-style function
    that zero/(-1)-initializes precisely these 8 offsets in this exact
    order. ARCHITECTURAL FINDING: 2011 unifies NewRoom/InvScreen/
    RestoreGame(Dialog)/RunDialog/RestartGame/SaveGame(Dialog)/RunAGSGame
    (9 `PostScriptAction` enum cases, `acruntim.h:686-695`) into one
    generic `postScriptActions[MAX_QUEUED_ACTIONS=5]` queue array (~725
    bytes, including a 500-byte `postScriptSaveSlotDescription[5][100]`).
    2002 instead gives 5 of those 9 cases (all but SaveGame/
    SaveGameDialog/RunAGSGame) their OWN dedicated struct field -- the
    generic queueing system is a later addition, not a reduced version of
    something already present. SaveGame/SaveGameDialog/RunAGSGame have no
    dedicated field anywhere in this 108-byte struct, so are either
    handled immediately (not deferred) in this build, or postdate it
    entirely.
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
  char _pad1[0x0C];       // +0x30..0x3C, unknown
  int numobjs;             // +0x3C, high confidence: confirmed via GUIMain::get_control_type's bounds
                           // check ("if (indx<0 || indx>=numobjs) return -1;").
  char _pad2[0x14];       // +0x40..0x54, unknown
  int mouseover;          // +0x54, confirmed via GUIMain::mouse_but_down
  char _pad3[0x08];       // +0x58..0x60, unknown
  int mousedownon;        // +0x60, confirmed via GUIMain::mouse_but_up/down
  char _pad4[0x2C];       // +0x64..0x90, unknown
  int on;                 // +0x90, confirmed via remove_popup_interface + is_mouse_on_gui
  void *objs[30];          // +0x94, high confidence. CORRECTED from an earlier `objs[60]` guess (inferred
                           // purely from total-struct-size arithmetic before objrefptr[] below was known to
                           // exist -- wrong). Elements point to GUIObject-derived instances; virtual method
                           // table has MouseDown at +0xC and MouseUp at +0x10 (not modeled here -- IDA
                           // structs don't hold vtables).
  int objrefptr[30];        // +0x10C, high confidence: confirmed via GUIMain::get_control_type reading
                           // (objrefptr[indx]>>16)&0xffff -- packed type+index data used to rebuild objs[]
                           // (see GUIMain::rebuild_array, already matched). objs[]/objrefptr[] are separate
                           // parallel MAX_OBJS_ON_GUI-sized arrays per source (Common/acgui.h:684-685), and
                           // MAX_OBJS_ON_GUI=30 here matches 2011's value (Common/acgui.h:654) exactly --
                           // zero drift, unlike most other fixed-capacity constants found in this project.
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
  int x;                        // +0x08, high confidence (GUIObject base field): CORRECTED from opaque
                            // padding -- resolved via GUITextBox::Draw's wrectangle(x,y,x+wid-1,y+hit-1)
                            // call (a sibling GUIObject-derived class; same base-subobject layout applies
                            // to GUIButton under the C++ ABI's single-inheritance field ordering).
  int y;                        // +0x0C, high confidence (GUIObject base field), see x above.
  int wid;                      // +0x10, high confidence (GUIObject base field), see x above --
                            // independently reconfirmed via GUITextBox::KeyPress's text-width bound check.
  int hit;                      // +0x14, high confidence (GUIObject base field), see x above.
  char _pad_unknown[0x04];    // +0x18..0x1C, unknown (GUIObject base field, still unconfirmed)
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

struct GUITextBox {
  void *vtbl;                  // +0x00, implicit
  unsigned int flags;           // +0x04, GUIObject base field (positional, carried from GUIButton's
                            // independently-confirmed evidence -- not re-verified for GUITextBox itself).
  int x;                        // +0x08, high confidence (GUIObject base field): confirmed directly via
                            // GUITextBox::Draw's wrectangle(x,y,x+wid-1,y+hit-1) call.
  int y;                        // +0x0C, high confidence, see x above.
  int wid;                      // +0x10, high confidence, see x above -- independently reconfirmed via
                            // GUITextBox::KeyPress's "wgettextwidth(text,font) > wid-(6+...)" bound check.
  int hit;                      // +0x14, high confidence, see x above.
  char _pad_unknown[0x04];    // +0x18..0x1C, unknown (GUIObject base field, still unconfirmed)
  int activated;                // +0x1C, high confidence (GUIObject base field): confirmed via
                            // GUITextBox::KeyPress's "activated++" on kp==13 (Enter).
  char text[200];               // +0x20, high confidence: fwrite/fread with ElementSize=0xC8(200),
                            // ElementCount=1 at this offset in GUITextBox::WriteToFile/ReadFromFile;
                            // also the strcpy destination in SetTextBoxText (already matched). Matches
                            // 2011's char text[200] with zero drift (unlike GUILabel below).
  int font;                     // +0xE8, high confidence: confirmed via WriteToFile/ReadFromFile's 3-int
                            // fwrite/fread block starting here, and Draw's check_font(&font) call.
  int textcol;                  // +0xEC, high confidence: confirmed via ReadFromFile's
                            // "if (textcol==0) textcol=16;" and Draw's wtextcolor/wsetcolor calls.
  int exflags;                  // +0xF0, high confidence: 3rd int in the font/textcol/exflags
                            // fwrite/fread block; independently corroborated by Draw's
                            // "(exflags & GTF_NOBORDER)" != 0 border-skip check.
  // Confirmed size so far: 0xF4 (minimum -- not verified against an allocation site).
};

struct GUILabel {
  void *vtbl;                  // +0x00, implicit
  unsigned int flags;           // +0x04, GUIObject base field (positional, see GUITextBox above).
  int x;                        // +0x08, GUIObject base field (positional, see GUITextBox above --
                            // not independently re-verified for GUILabel specifically this round).
  int y;                        // +0x0C, positional, see x above.
  int wid;                      // +0x10, positional, see x above.
  int hit;                      // +0x14, positional, see x above.
  char _pad_unknown[0x04];    // +0x18..0x1C, unknown (GUIObject base field, still unconfirmed)
  int activated;                // +0x1C, positional (GUIObject base field, see GUITextBox above).
  char text[200];               // +0x20, high confidence: fwrite/fread with ElementSize=0xC8(200),
                            // ElementCount=1 at this offset in GUILabel::WriteToFile/ReadFromFile.
                            // IMPORTANT DRIFT: this is a FIXED inline array in this 2002 build. 2011
                            // replaced it with a dynamically-allocated `char *text` + `textBufferLen`
                            // (Common/acgui.h:287-288) -- the old fixed-array fwrite survives only as a
                            // dead commented-out line in 2011's WriteToFile ("//fwrite(&text[0],
                            // sizeof(char), 200, ooo);", acgui.cpp:276), and 2011's ReadFromFile
                            // explicitly branches "if (version<113) textBufferLen=200;" (acgui.cpp:289)
                            // -- both directly confirm this build predates that refactor.
  int font;                     // +0xE8, high confidence: confirmed via WriteToFile/ReadFromFile's 3-int
                            // fwrite/fread block starting here (mirrors GUITextBox's layout exactly).
  int textcol;                  // +0xEC, high confidence: confirmed via ReadFromFile's
                            // "if (textcol==0) textcol=16;".
  int align;                    // +0xF0, high confidence (positional, 3rd int in the block): matches
                            // source's "int font,textcol,align;" (Common/acgui.h:290).
  // Confirmed size so far: 0xF4 (minimum -- not verified against an allocation site).
};

struct GUIListBox {
  void *vtbl;                  // +0x00, implicit
  unsigned int flags;           // +0x04, GUIObject base field (positional, see GUITextBox above).
  int x;                        // +0x08, GUIObject base field (positional, see GUITextBox above).
  int y;                        // +0x0C, positional, see x above.
  int wid;                      // +0x10, positional, see x above.
  int hit;                      // +0x14, positional, see x above.
  char _pad_unknown[0x04];    // +0x18..0x1C, unknown (GUIObject base field, still unconfirmed)
  int activated;                // +0x1C, positional (GUIObject base field, see GUITextBox above --
                            // not independently re-verified for GUIListBox specifically this round).
  char _pad_items[0x190];     // +0x20..0x1B0, unknown: this is `items[MAX_LISTBOX_ITEMS]` (char*) and
                            // `saveGameIndex[MAX_LISTBOX_ITEMS]` (short) per source (acgui.h:386-387), but
                            // the two arrays are NOT individually split out here -- naive 6-byte-per-entry
                            // (4-byte ptr + 2-byte short) arithmetic against this region's size doesn't
                            // divide evenly, so the exact 2002 MAX_LISTBOX_ITEMS/layout isn't pinned down
                            // yet. Left as opaque padding rather than a guessed split.
  int numItems;                 // +0x1B0, high confidence: first field of an 11-int block confirmed via
                            // GUIListBox::WriteToFile's 44-byte bulk fwrite (ElementSize=0xB,Count=4)
                            // starting here, matching source's declared order exactly (acgui.h:388-390).
  int selected;                  // +0x1B4, high confidence, see numItems above.
  int topItem;                   // +0x1B8, high confidence, see numItems above.
  int mousexp;                   // +0x1BC, high confidence: independently confirmed via
                            // GUIListBox::MouseMove's "mousexp=nx-x;" AND via the WriteToFile block above
                            // landing at exactly this offset -- two independent confirmations agreeing.
  int mouseyp;                   // +0x1C0, high confidence, see mousexp above (same double confirmation).
  int rowheight;                 // +0x1C4, high confidence, see numItems above.
  int num_items_fit;             // +0x1C8, high confidence, see numItems above.
  int font;                      // +0x1CC, high confidence, see numItems above.
  int textcol;                   // +0x1D0, high confidence, see numItems above.
  int backcol;                   // +0x1D4, high confidence, see numItems above.
  int exflags;                   // +0x1D8, high confidence, see numItems above.
  // DRIFT: 2011 declares 3 more fields right after exflags (selectedbgcol, alignment, reserved1,
  // acgui.h:391-392) that are NOT part of this build's WriteToFile bulk write -- absent or not yet
  // persisted in this 2002 build. Confirmed size so far: 0x1DC (minimum).
};

struct GUIInv {
  void *vtbl;                  // +0x00, implicit
  unsigned int flags;           // +0x04, GUIObject base field (positional, see GUITextBox above).
  int x;                        // +0x08, GUIObject base field (positional, see GUITextBox above).
  int y;                        // +0x0C, positional, see x above.
  int wid;                      // +0x10, positional, see x above.
  int hit;                      // +0x14, positional, see x above.
  char _pad_unknown[0x04];    // +0x18..0x1C, unknown (GUIObject base field, still unconfirmed)
  int activated;                // +0x1C, high confidence (GUIObject base field): confirmed via
                            // GUIInv::MouseUp's "if (isover) activated=1;".
  int isover;                    // +0x20, high confidence: GUIInv's own first declared field
                            // (Common/acgui.h:467), confirmed via MouseOver ("isover=1;"), MouseLeave
                            // ("isover=0;"), and MouseUp (reads it) all agreeing on this offset.
  // DRIFT: this build's WriteToFile/ReadFromFile touch ONLY the GUIObject base block -- no charId/
  // itemWidth/itemHeight/topIndex read or written at all (2011's own source shows these are gated
  // behind "if (version>=109)", acgui.h:486-497 -- this build predates that format version). Whether
  // those 4 fields exist in this build's struct at all (just unpersisted) or don't exist yet is NOT
  // resolved -- deliberately not guessed here. Confirmed size so far: 0x24 (minimum).
};

struct GUISlider {
  void *vtbl;                  // +0x00, implicit
  unsigned int flags;           // +0x04, GUIObject base field (positional, see GUITextBox above).
  int x;                        // +0x08, GUIObject base field (positional, see GUITextBox above).
  int y;                        // +0x0C, positional, see x above.
  int wid;                      // +0x10, positional, see x above.
  int hit;                      // +0x14, positional, see x above.
  char _pad_unknown[0x04];    // +0x18..0x1C, unknown (GUIObject base field, still unconfirmed)
  int activated;                // +0x1C, positional (GUIObject base field, see GUITextBox above --
                            // not independently re-verified for GUISlider specifically this round).
  int min;                       // +0x20, high confidence: confirmed via GUISlider::WriteToFile's
                            // 16-byte bulk fwrite starting here (min/max/value/mpressed), and
                            // GUISlider::Draw's opening "if (min>=max) max=min+1;" read.
  int max;                       // +0x24, high confidence, see min above.
  int value;                     // +0x28, high confidence, see min above.
  int mpressed;                  // +0x2C, high confidence: confirmed THREE independent ways --
                            // GUISlider::MouseDown sets it 1, MouseUp clears it 0, MouseMove guards
                            // on it ("if (mpressed==0) return;") before its drag-ratio float math --
                            // plus agrees exactly with the WriteToFile bulk-write offset.
  // DRIFT: the 3 fields declared right after mpressed in 2011 (handlepic, handleoffset, bgimage,
  // Common/acgui.h:218) are NOT part of this build's WriteToFile bulk write -- absent or persisted
  // differently. Confirmed size so far: 0x30 (minimum).
};

struct SpriteCache {
  long *offsets;                // +0x00, high confidence: confirmed via SpriteCache::initFile's
                            // "offsets[vv]=0;" loop write (Common/sprcache.cpp:642).
  long elements;                 // +0x04, high confidence: confirmed as the loop bound in the same
                            // initFile loop ("for (vv=0; vv<elements; vv++) ...").
  void **images;                 // +0x08, high confidence: confirmed via initFile's "images[vv]=NULL;"
                            // loop write (sprcache.cpp:641). (block == BITMAP*, already typedef'd above,
                            // used loosely here as void* since the exact block typedef isn't in scope
                            // at this point in the decl order -- functionally equivalent.)
  void *ff;                      // +0x0C, high confidence: confirmed via initFile's
                            // "ff=clibfopen(filnam,\"rb\");" assignment (sprcache.cpp:645).
  // Total size EXACTLY 0x10 (16 bytes), matching the IDB's pre-existing known struct size --
  // this struct is COMPLETE, not a minimum/partial recovery. DRASTIC DRIFT from 2011's
  // ~16-member class SpriteCache (Common/sprcache.h): none of the LRU-eviction bookkeeping
  // (mrulist, mrubacklink, liststart, listend, lastLoad, maxCacheSize, lockedSize), per-sprite
  // metadata (sizes, flags, spritesAreCompressed), or cache-size accounting (cachesize) exist in
  // this 2002 build -- the whole discardable-cache subsystem is a later addition. 2002's
  // SpriteCache is just two raw parallel arrays (offsets/images) plus a count and a file handle,
  // with no size limit or eviction policy.
};

struct EventBlock {
  int list[8];                    // +0x00, high confidence: MAXCOMMANDS=8 (Common/acroom.h:238).
                            // Confirmed via run_event_block (sub_417088), which loops
                            // "for(i=0;i<numcmd;i++)" reading "[this+i*4]" and comparing it
                            // against the caller-supplied checkAgainst/matchType codes -- an
                            // event/command-type code per entry.
  int respond[8];                 // +0x20, high confidence: confirmed via run_event_block reading
                            // "[this+i*4+0x20]" and switching on its value: 1 (a no-op branch),
                            // 2 (call StopMoving(playerchar)), 3 (call run_on_event(3,data[i])),
                            // 5 (unexplored branch) -- a response-TYPE code per entry.
  int respondval[8];               // +0x40, high confidence: confirmed via run_event_block
                            // reading "[this+i*4+0x40]" and passing it directly as the `msnum`
                            // argument to the already-matched DisplayMessage -- the response
                            // VALUE (here, a message number) per entry.
  int data[8];                    // +0x60, high confidence: confirmed via run_event_block reading
                            // "[this+i*4+0x60]" for extra-data comparisons (LoseInventory gating)
                            // and as the wparam argument to run_on_event -- extra per-entry data.
  int numcmd;                     // +0x80, high confidence: confirmed as the loop bound in
                            // run_event_block ("cmp edx,[this+0x80]; jge <done>") -- count of
                            // populated entries in the 8-slot arrays above.
  short score[8];                  // +0x84, high confidence: confirmed via run_event_block
                            // reading "[this+i*2+0x84]" (2-byte stride), passing it to the
                            // already-matched GiveScore if nonzero, then immediately zeroing it
                            // -- a one-time per-entry score award, matching 2011's documented
                            // semantics exactly.
  // Total confirmed size 0x94 (148 bytes), matching 2011's declared EventBlock (acroom.h:239-246)
  // with ZERO drift in every field's type, order, and offset -- fully confirmed via
  // run_event_block's own body, not just an arithmetic/positional fit. Used as
  // GameSetupStructBase::__charcond[50]/__invcond[100] below, both independently confirmed via
  // their own access sites (RunCharacterInteraction / run_event_block_inv).
};

struct MouseCursor {
  int pic;                        // +0x00, high confidence: confirmed via ChangeCursorGraphic
                            // (already matched) writing newslot into `dword_51585C[curs]`
                            // (`imul curs,18h`), and via a cursor-precache loop in `main` reading
                            // it to call the already-matched SpriteCache::precache. Matches 2011's
                            // `int pic` (Common/acroom.h:2456) with zero drift.
  short hotx;                     // +0x04, high confidence: confirmed via SetMouseCursor (already
                            // named, retroactively documented -- see matches.json) reading
                            // `word_515860[curs]` into a local later used as the cursor's hotspot
                            // x-coordinate, and via ChangeCursorHotspot (already matched) writing
                            // it. Address delta from `pic` (`0x515860-0x51585C=4`) exactly matches
                            // the predicted field size. Zero drift from 2011's `short hotx`.
  short hoty;                     // +0x06, high confidence: same evidence pattern as hotx
                            // (SetMouseCursor reads `word_515862[curs]`, ChangeCursorHotspot
                            // writes it). Address delta from hotx (`0x515862-0x515860=2`) exactly
                            // matches the predicted field size. Zero drift from 2011's `short hoty`.
  short view;                     // +0x08, high confidence: confirmed via __GetLocationType
                            // (already matched) reading `word_515864[cur_cursor]`, and via a
                            // cursor-precache loop in `main` passing the same field directly to
                            // the already-matched `precache_view` -- an exact semantic match
                            // (a "view" field feeding a view-precaching function). Address delta
                            // from hoty (`0x515864-0x515862=2`) exactly matches. Zero drift from
                            // 2011's `short view`.
  char name[10];                  // +0x0A, MEDIUM confidence: positional only -- zero references
                            // anywhere in the disassembly (plausible for an editor-only
                            // descriptive label the runtime never reads, same pattern as
                            // GameSetupStructBase's `target_win`). Included because it's boxed in
                            // with zero slack between the confirmed `view` (ends +0x0A) and
                            // `flags` (starts +0x14) fields -- 10 bytes, exactly matching 2011's
                            // `char name[10]` (acroom.h:2459). NOTE: this exact 10-byte span is
                            // where IDA's .asm export emits a spurious "align 10h" directive for
                            // element 0 specifically (see GameSetupStructBase::mcurs's comment,
                            // and struct-layout-drift.md) -- IDA's own heuristic mislabeling of
                            // real (if unreferenced) struct content, not a genuine compiler gap.
  char flags;                     // +0x14, high confidence: confirmed via sub_40D2F4 (called from
                            // the already-matched SetNextCursorMode/SetCursorMode, not itself
                            // renamed): `byte_515870[curs] & 2` and `& 4` bitmask checks match
                            // 2011's `MCF_DISABLED=2`/`MCF_STANDARD=4` (acroom.h:2451-2452)
                            // exactly. Zero drift from 2011's `char flags`.
  // Total confirmed size: 0x15 (21 bytes) of fields, but the confirmed STRIDE (via `imul reg,18h`
  // in every indexing site) is 0x18 (24 bytes) -- the remaining 3 bytes are natural compiler
  // alignment padding after `char flags` (bringing the struct up to 4-byte alignment for the
  // array), not a missing field. This struct is a RARE case in this project: a full match to
  // 2011's declared layout with genuinely ZERO drift in every field found, both size and order.
  // Defined here (moved up from its original position after GameSetupStructBase) because
  // GameSetupStructBase now embeds `MouseCursor mcurs[10]` directly as a real array member, which
  // requires the complete type in scope (unlike ccScript's forward declaration above, which is
  // only ever used behind a pointer).
  char _pad_align[3];             // +0x15..0x18, compiler alignment padding (not a real field)
};

struct InterfaceElement {
  // Total size 0x334 (820 bytes), high confidence -- independently confirmed via its own array
  // stride (`imul reg,334h` in load_ac2game_dta, RunCharacterInteraction-adjacent code) AND via
  // the byte-for-byte match on the two fields below, matching 2011's `InterfaceElement`
  // (Common/acroom.h:304-320) with zero drift on total size. Only the LAST two fields have
  // independent access-site evidence so far -- the rest (x/y/x2/y2, bgcol/fgcol/bordercol,
  // vtextxp/vtextyp/vtextalign, vtext[40], numbuttons, button[20], flags,
  // reserved_for_future, popupyp) are NOT yet independently confirmed field-by-field, so are
  // left as opaque padding rather than asserted -- per this project's "don't assert past your
  // evidence" rule (unlike EventBlock, whose every field WAS independently confirmed via
  // run_event_block before being fully typed). `button[MAXBUTTON=20]` alone would need its own
  // `InterfaceButton` struct (acroom.h:291-301) to type properly, which has zero access-site
  // evidence of its own yet -- left undefined until/unless a future round finds one.
  char _unconfirmed[0x330];       // +0x00..0x330 (816 bytes), matches 2011's declared layout by
                            // position (x/y/x2/y2 through popupyp, acroom.h:305-313) but no
                            // field within it has independent access-site evidence yet.
  char popup;                     // +0x330, high confidence: `byte_513B7C`. Confirmed via
                            // load_ac2game_dta (already matched): "imul edx,334h;
                            // movsx eax,byte_513B7C[edx]; cmp eax,2" -- byte_513B7C's own
                            // address sits EXACTLY at offset 0x330 within the newly-confirmed
                            // `iface[10]` array (see GameSetupStructBase::iface below), matching
                            // 2011's `char popup;` (acroom.h:314) at its exact declared offset
                            // with zero drift -- the strongest single piece of evidence that
                            // this whole struct's layout matches 2011's byte-for-byte.
  char on;                        // +0x331, high confidence: `byte_513B7D`. Confirmed the same
                            // way, immediately following `popup` -- matches 2011's `char on;`
                            // (acroom.h:315) at its exact declared offset with zero drift. Set
                            // to 0 or 1 in load_ac2game_dta depending on whether the
                            // corresponding `popup` field equals 2, consistent with 2011's
                            // constructor default ("on = 1;", acroom.h:318) being conditionally
                            // overridden during game-data load.
  char _pad_align[2];             // +0x332..0x334, likely compiler alignment padding (not
                            // independently confirmed as real content) -- boxed in with zero
                            // slack by the confirmed total stride (0x334) and the confirmed `on`
                            // field ending at +0x332.
};

struct GameSetupStructBase {
  char gamename[30];             // +0x00, high confidence: this IS the struct's base address
                            // (the global instance is literally named `game_gamename` in the IDB,
                            // confirmed via load_ac2game_dta's bulk fread into it). DRIFT: 2011 has
                            // char gamename[50] (Common/acroom.h:2817) -- only 30 bytes here.
  unsigned char options[20];      // +0x1E, high confidence: byte_513337 (index 1) confirmed via
                            // GiveScore's "if (amnt>0 && options[OPT_SCORESOUND]>0)
                            // PlaySound(options[OPT_SCORESOUND]);" pattern -- OPT_SCORESOUND=1
                            // (Common/acroom.h:2707) matches its position exactly (1 byte after
                            // gamename's 30-byte end). True SIZE (20, not just the ~19 individually
                            // XREF'd bytes) confirmed by ARITHMETIC against paluses' independently-
                            // confirmed start below (0x32 - 0x1E = 0x14 = 20), not by trusting the
                            // labels' own declared extents (see methodology note above). DRIFT: 2011
                            // has `int32 options[100]` (400 bytes) -- this 2002 build uses single
                            // BYTES per option (not 4-byte ints) and has room for only 20 of them,
                            // not 100 -- both a type change and a ~20x size reduction. The highest
                            // OPT_* constant that could fit is index 19 (OPT_FADETYPE, acroom.h:2725)
                            // -- consistent with this build predating later UI/rendering options
                            // (OPT_DIALOGNUMBERED, OPT_MOUSEWHEEL, OPT_ANTIALIASFONTS, etc.).
  unsigned char paluses[256];     // +0x32, high confidence: confirmed directly via a shared loop in
                            // `main` (also used for defpal below) -- "for (ee=0; ee<256; ee++) if
                            // (paluses[ee]!=2) palette[ee]=defpal[ee];" -- disasm's `cmp
                            // [ee],100h` loop bound and `cmp edx,2` (PAL_BACKGROUND) check match
                            // source's palette-slot-usage semantics exactly. Matches 2011's
                            // `unsigned char paluses[256]` (acroom.h:2819) with ZERO drift -- both
                            // size and position.
  // defpal (2011: `color defpal[256]`, acroom.h:2820) is DELIBERATELY NOT included here despite
  // strong positional evidence -- a field matching "game.defpal[ee]" usage (Engine/AC.CPP:26196-
  // 26198's init_game_settings loop: "for(ee=0;ee<256;ee++) if(paluses[ee]!=PAL_BACKGROUND)
  // palette[ee]=defpal[ee];") does start at +0x132, right after paluses. BUT the loop's own
  // 256-element reach does NOT mean the underlying array is actually 256 dwords (1024 bytes)
  // here: the raw disassembly shows only 51 dwords (204 bytes) plus 2 trailing bytes of
  // confirmed adjacent space before running into a COMPLETELY UNRELATED global
  // (g_interface, AGS's legacy script-exported "interface" object, registered via
  // scAdd_External_Symbol -- just placed memory-adjacent by the linker, not a struct field at
  // all). 206 bytes isn't even a clean multiple of 4, so this can't be pinned to a specific
  // element count either. Most likely explanation: 2002's defpal[] is genuinely much smaller
  // than 256 (consistent with the fixed-capacity-shrinkage pattern seen everywhere else in this
  // project), and the loop's out-of-bounds reads for higher `ee` simply never trigger in
  // practice because this game's actual paluses[] data never sets a non-background flag past
  // whatever defpal's true (smaller) capacity is. This was originally (wrongly) recorded as a
  // confirmed `unsigned long defpal[256]` with "zero drift" -- RETRACTED once the raw byte
  // layout was checked mechanically; see the correction in struct-layout-drift.md for the full
  // reasoning. Do not re-add a defpal field without independently pinning down its true size
  // (e.g. finding an allocation-size constant, or another access site with a tighter, confirmed
  // bound) -- position alone (matches at +0x132) is not suffient evidence for size.
  //
  // Beyond this point, the FIELD ORDER itself is no longer assumed to match 2011's declaration
  // order -- see the 5 fields below, whose confirmed positions actively DISPROVE a simple
  // "same order, just bigger" hypothesis (chars sits far EARLIER, relative to playercharacter,
  // than 2011's base+derived split would predict, and numcharacters sits DIRECTLY adjacent to
  // chars rather than near numviews/playercharacter as 2011 declares it -- numviews itself sits
  // well separated from both). Each confirmed field below is therefore treated as an
  // independently-anchored island, with explicit unknown-content padding on both sides rather
  // than assumed adjacency to its 2011 declared
  // neighbors.
  int defpal[256];                 // +0x132..0x532 (1024 bytes), high confidence: promoted from
                            // a RETRACTED hypothesis to CONFIRMED this round. `dword_51344A`.
                            // The original retraction (see struct-layout-drift.md) was based
                            // purely on IDA's declared label extent (~204 bytes before hitting
                            // what IDA calls `g_interface`) -- but that check never verified
                            // whether the actual CODE indexes further than that boundary. It
                            // does: `main` (already matched) has its OWN copy of "for(ee=0;
                            // ee<256;ee++) if(paluses[ee]!=PAL_BACKGROUND) palette[ee]=
                            // defpal[ee];" (AC.CPP:26196-26198) as "cmp byte_51334A[ecx],2; jz
                            // <skip>; mov ecx,dword_51344A[eax*4]; mov palette[edx*4],ecx" --
                            // an UNCONDITIONAL 4-byte-stride read up to index 255, addressing
                            // 1024 bytes total starting at `dword_51344A`, matching 2011's
                            // `color defpal[256]` (acroom.h:2773) in BOTH type (4-byte packed
                            // color, not a smaller/different encoding) and count with ZERO
                            // drift. This addressable range extends well past where IDA's
                            // `g_interface`/`byte_513B7C`/`byte_513B7D` labels begin -- since
                            // `game_gamename` is one single contiguous allocation (confirmed via
                            // its own `sizeof()`-based `fread` call), those labels cannot be
                            // genuinely separate globals if their addresses fall inside this
                            // already-established 1024-byte span; they're simply IDA's own
                            // (mistaken) sub-labeling of real `defpal`/`iface[10]` content --
                            // see struct-layout-drift.md for the full resolution. NOTE: a
                            // SEPARATE, unrelated function (`load_new_room`) has its own
                            // near-identical loop reading from a genuinely different global
                            // (`dword_51F69C`) -- do not confuse the two; only `main`'s copy
                            // reads from inside `GameSetupStructBase`.
  char _pad_align2[2];            // +0x532..0x534, compiler alignment padding (not a real
                            // field) -- `defpal[256]`'s own 1024-byte extent lands 2 bytes short
                            // of 4-byte alignment, and `InterfaceElement`'s int-heavy layout
                            // needs 4-byte alignment for its own fields -- boxed in with zero
                            // slack between `defpal`'s confirmed end and `iface[10]`'s confirmed
                            // start.
  InterfaceElement iface[10];      // +0x534..0x253C (8200 bytes), high confidence: ALSO promoted
                            // from a documented-but-unconfirmed hypothesis to CONFIRMED this
                            // round. A perfect zero-slack fit was already known (10*0x334 lands
                            // exactly on the already-confirmed `numiface`'s start) but this round
                            // added real field-level evidence: `byte_513B7C`'s own address sits
                            // EXACTLY at offset 0x330 within this array -- matching
                            // `InterfaceElement.popup`'s declared offset (acroom.h:314) to the
                            // byte -- and `byte_513B7D` (immediately following) matches `.on`
                            // (acroom.h:315) the same way. Matches OriGameSetupStruct's declared
                            // "InterfaceElement iface[10]; int numiface;" adjacency
                            // (acroom.h:2774-2775) exactly, with ZERO drift in element count
                            // (unlike most fixed-capacity arrays elsewhere in this project, which
                            // usually shrink). See the `InterfaceElement` struct above for which
                            // of its OWN internal fields are independently confirmed (only
                            // `popup`/`on` so far -- the rest is positionally boxed in but not
                            // asserted field-by-field).
  int numiface;                   // +0x253C, high confidence: `dword_515854`. Confirmed two
                            // ways: (1) POSITION -- sits with zero gap immediately before
                            // `numviews` (ends exactly at +0x2540), matching OriGameSetupStruct's
                            // declared "int numiface; int numviews;" adjacency (acroom.h:2775-
                            // 2776) exactly, the same "count field right before the next field"
                            // idiom seen throughout this struct (numcharacters/chars, numgui/
                            // dict, numdialog/numdlgmessage/numfonts). (2) USAGE -- inside
                            // load_ac2game_dta (already matched): "cmp ecx,dword_515854; jge
                            // <done>" gates a loop indexing `byte_513B7C[i*0x334]`/
                            // `byte_513B7D[i*0x334]` (0x334=820=InterfaceElement's independently
                            // -confirmed size, see the arithmetic above) -- an exact match to
                            // "count of populated interface elements" semantics. This was
                            // originally investigated as a `numdialog` candidate and dropped
                            // (correctly -- `numdialog` was later confirmed elsewhere at
                            // +0x9FCC) without anyone revisiting it as a `numiface` candidate,
                            // since `OriGameSetupStruct`'s identity wasn't known yet at the time.
  int numviews;                   // +0x2540, high confidence: this is the pre-existing IDB label
                            // `ElementCount` (a generic name, apparently IDA-auto-assigned from
                            // being reused as a size_t parameter across unrelated fread/fwrite
                            // calls elsewhere -- kept as-is rather than renamed, since it's a data
                            // label already in wide incidental use, not a function). Confirmed as
                            // `numviews` via SetObjectFrame (already matched, AC.CPP:14635-14653):
                            // "if (viw>=game.numviews) quit(...)" matches disasm's "viw--; cmp
                            // viw,ElementCount; jl <ok>" exactly.
  MouseCursor mcurs[10];           // +0x2544..0x2634 (240 bytes), high confidence: promoted from
                            // generic padding to CONFIRMED this round. `dword_51585C` (element
                            // 0's `pic` field) sits EXACTLY where this padding used to start --
                            // MouseCursor's own struct was already fully recovered in an earlier
                            // round (see the `MouseCursor` struct below) via `ChangeCursorGraphic`
                            // /`SetMouseCursor`/`__GetLocationType`/a cursor-precache loop in
                            // `main`, all confirming a stable `imul reg,18h` (24-byte) array
                            // stride -- this round just traced that array's base address back to
                            // its position INSIDE GameSetupStructBase for the first time. Exact
                            // triple confirmation: (1) start matches `dword_51585C` exactly; (2)
                            // 10*0x18=0xF0 matches this pad's size exactly; (3) end (+0x2634)
                            // matches the already-confirmed `globalscript`'s start with zero gap.
                            // Cross-checked element-by-element: `dword_5158BC`/`word_5158C0`/
                            // `word_5158C2` (individually labeled, likely because cursor index 4
                            // = MODE_USE gets special-cased hotdot-marker handling elsewhere) sit
                            // EXACTLY at (0x25A4-0x2544)/0x18 = element index 4's `pic`/`hotx`/
                            // `hoty` offsets, independently confirming the stride holds across
                            // the whole array, not just element 0. Matches OriGameSetupStruct's
                            // declared `MouseCursor mcurs[10];` (acroom.h:2777) exactly, in both
                            // identity and position (right after `numiface`/`numviews`, matching
                            // 2011's/OriGameSetupStruct's declared adjacency). NOTE: the byte
                            // range +0x254E..+0x2558 (element 0's unreferenced `name[10]` field)
                            // is where IDA emits a spurious "align 10h" directive in the .asm --
                            // this is IDA's own heuristic mislabeling of real (if
                            // never-referenced) struct content as alignment filler, not a genuine
                            // compiler-inserted gap; see struct-layout-drift.md for the full
                            // reasoning (this does NOT invalidate the separately-verified +8-byte
                            // OFFSET CORRECTION below `dict` -- that correction's byte-count is
                            // still exactly right regardless of how this one gap is interpreted).
  char *globalscript;             // +0x2634, medium-high confidence: `dword_51594C`. Identified via
                            // restore_game_data (already matched): before re-`fread`-ing the whole
                            // struct from a save file, the function explicitly SAVES this field off
                            // to a local, then RESTORES it afterward -- alongside the same-pattern
                            // `compiled_script` (below) and `chars`/`numcharacters` (confirmed
                            // above) -- a "preserve compiled game assets across a savegame restore"
                            // idiom (a save file carries game STATE, not a fresh copy of the
                            // compiled script/character data, so these pointers/counts must survive
                            // the bulk overwrite). No independent access site of its own beyond this
                            // preserve/restore pair, but the position is a PERFECT zero-slack fit:
                            // sits immediately before numcharacters (ends exactly at +0x2638, zero
                            // gap), which itself sits immediately before chars -- reproducing 2011's
                            // OriGameSetupStruct declared sequence "char *globalscript; int
                            // numcharacters; OldCharacterInfo *chars;" (acroom.h:2778-2780) with
                            // ZERO drift in field order across all three fields. See the
                            // OriGameSetupStruct identity discovery in struct-layout-drift.md.
  int numcharacters;              // +0x2638, high confidence: confirmed via is_valid_character
                            // (AC.CPP:3820, already matched -- see matches.json): "if
                            // ((newchar<0) || (newchar>=game.numcharacters)) return 0;" matches
                            // disasm's "cmp arg_0,0" / "cmp ecx,dword_515950" pair exactly.
                            // Sits DIRECTLY adjacent to chars below (ends exactly at +0x263C,
                            // zero gap) -- notable, since 2011 declares numcharacters and chars
                            // far apart (acroom.h:2822 vs :2842); this build apparently groups
                            // the character count immediately before the character array
                            // pointer, a natural "count-then-pointer" pairing that 2011's later
                            // refactor separated. Matches 2011's `int32 numcharacters` with zero
                            // type drift.
  void *chars;                   // +0x263C, high confidence: `CharacterInfo *chars`. Confirmed
                            // via SetPlayerCharacter (already matched): "game_chars +
                            // newchar*0x140" -- the 0x140 stride matches this project's
                            // independently-confirmed CharacterInfo struct size exactly (see
                            // CharacterInfo section above). Also confirmed via
                            // scAdd_External_Symbol("character", game_chars) registering the
                            // script-exposed `character[]` array pointer. Matches 2011's `chars`
                            // field (acroom.h:2842) in IDENTITY, but NOT in position -- 2011
                            // declares this near the very end of the base struct (after
                            // messages[]/dict/globalscript), yet here it sits at only +0x263C,
                            // long before playercharacter (+0x7CFC) below. This is direct
                            // evidence the 2002 field order genuinely differs from 2011's, not
                            // just the field sizes/types.
  EventBlock __charcond[50];      // +0x2640..0x4328 (7400 bytes), high confidence: promoted from
                            // arithmetic-fit hypothesis to CONFIRMED. `unk_515958`. Confirmed via
                            // the already-matched RunCharacterInteraction (AC.CPP:16341): builds
                            // "offset unk_515958 + cc*0x94" and passes it to the newly-identified
                            // run_event_block (sub_417088) as its EventBlock* argument -- address
                            // (0x515958) lands exactly on the zero-slack arithmetic-fit prediction
                            // (base+0x2640, immediately after `chars` with no gap), and the 0x94
                            // stride matches EventBlock's own independently-confirmed size (see
                            // the EventBlock struct below) exactly. Matches OriGameSetupStruct's
                            // declared field (acroom.h:2781) in identity and position.
  EventBlock __invcond[100];      // +0x4328..0x7CF8 (14800 bytes), high confidence: same
                            // confirmation as __charcond above. `unk_517640`. Confirmed via the
                            // already-matched run_event_block_inv (sub_40D8A9): its callers in
                            // check_controls all build "offset unk_517640 + iit*0x94" and pass it
                            // straight through to run_event_block. Address (0x517640) lands
                            // exactly at __charcond's end (zero gap), matching OriGameSetupStruct's
                            // declared adjacency "EventBlock __charcond[50]; EventBlock
                            // __invcond[100];" (acroom.h:2781-2782) exactly. Together, __charcond
                            // and __invcond account for the ENTIRE 22200-byte gap that used to sit
                            // between `chars` and `compiled_script` with zero bytes left over.
  void *compiled_script;         // +0x7CF8, high confidence: `ccScript *compiled_script`.
                            // `dword_51B010`. Confirmed via load_ac2game_dta (already matched): "cmp
                            // dword_51B010,0; jz skip; call fread_script(stream)" -- fread_script
                            // (Common/CSRUN.CPP:2029, already matched) is the deserializer for a
                            // compiled ccScript blob, gated by this field as a presence flag. ALSO
                            // confirmed via restore_game_data's save/restore-across-reload pattern
                            // (see globalscript above) -- both together nail down both position and
                            // identity. Matches 2011's `compiled_script` (acroom.h:2843) in identity;
                            // position differs (2011 declares it near the very end, after
                            // messages[]/dict/globalscript/chars) -- more evidence of 2002's
                            // divergent field order, consistent with the chars/playercharacter
                            // finding above.
  int playercharacter;           // +0x7CFC, high confidence: confirmed via GetPlayerCharacter
                            // (already matched, script-exported): "return game_playercharacter;"
                            // -- an exact, direct match with zero surrounding logic. Also written
                            // by SetPlayerCharacter. Matches 2011's `int32 playercharacter`
                            // (acroom.h:2823) with zero type drift.
  unsigned char _pad_unknown3[0x834]; // +0x7D00..0x8534, unknown (2100 bytes) -- almost certainly
                            // `unsigned char __old_spriteflags[2100]` (OriGameSetupStruct's
                            // declared order, acroom.h:2785, right after playercharacter) -- exact
                            // size match, but not independently confirmed via an access site this
                            // round (arithmetic fit only, per the defpal-retraction rule -- not
                            // asserted as a typed field).
  int totalscore;                 // +0x8534, high confidence: `dword_51B84C`. A PERFECT zero-slack
                            // positional fit (this field's whole 4 bytes exactly fills the gap
                            // between the __old_spriteflags-shaped span above and numinvitems
                            // below) PLUS direct disassembly confirmation: `main`'s huge inlined
                            // play-struct-initialization block (matching Engine/AC.CPP's
                            // "play.gamma_adjustment=100; ...; play.totalscore=game.totalscore;
                            // play.roomscript_finished=0; ..." sequence, AC.CPP:26320-26349) does
                            // "mov edx,dword_51B84C; mov dword_4EEB30,edx" -- an exact
                            // transliteration of "play.totalscore = game.totalscore;" (AC.CPP:26348),
                            // sitting in the middle of a long chain of other already-recognizable
                            // `play.x = <literal>;` assignments. Matches 2011's `int32 totalscore`
                            // (acroom.h:2824) with zero type drift.
  short numinvitems;              // +0x8538, high confidence: confirmed via its `movsx` (sign-
                            // extended from a 2-byte WORD) use as the inventory-index upper
                            // bound in multiple already-matched functions (GetInvName,
                            // update_invorder, SetInvItemPic, and others). Matches 2011's
                            // `short numinvitems` (acroom.h:2825) with ZERO type drift -- both
                            // are the same signed 16-bit field.
  char _pad_unknown4[0x1A92];    // +0x853A..0x9FCC, unknown (6802 bytes) -- likely contains
                            // color_depth, target_win, dialog_bullet, hotdot/hotdotouter,
                            // uniqueid, and/or more of the derived GameSetupStruct's fixed
                            // arrays, unconfirmed.
  int numdialog;                  // +0x9FCC, high confidence: confirmed via RunDialog (already
                            // matched, AC.CPP:16065-16067): "if ((tum<0) | (tum>=game.numdialog))
                            // quit(\"!RunDialog: invalid topic number specified\");" -- disasm's
                            // "cmp arg_0,0; setl al" / "cmp ecx,dword_51D2E4; setnl dl" / "or eax,edx"
                            // pattern is byte-for-byte identical in shape to the already-confirmed
                            // is_valid_character (numcharacters) match -- same compiler idiom for a
                            // bitwise-OR two-sided range check. Matches 2011's `int32 numdialog`
                            // (acroom.h:2826) with zero type drift. NOTE: this is NOT the same
                            // field as `dword_515854` -- an earlier round correctly ruled that
                            // OUT as `numdialog` (its indexed array target didn't match
                            // `DialogTopic`'s size at all), but a LATER round identified what
                            // `dword_515854` actually IS: `numiface` (see above) -- see
                            // struct-layout-drift.md for both writeups.
  int numdlgmessage;               // +0x9FD0, high confidence: `dword_51D2E8`. Confirmed via
                            // load_ac2game_dta (already matched): "cmp dword_51D2E8,7D0h; jle <ok>;
                            // push offset \"Error in game file: too many dialog lin\"...; call
                            // quit" -- a hard cap check (2000) exactly matching the concept of a
                            // global-dialog-message count limit -- followed by a loop
                            // "for(i=0;i<dword_51D2E8;i++) malloc(0x1F4=500)" allocating one
                            // 500-byte buffer per message. Sits immediately after numdialog with
                            // zero gap, matching 2011's comma-declared "int32 numdialog,
                            // numdlgmessage;" (acroom.h:2826) exactly -- both position and
                            // semantics confirmed.
  int numfonts;                   // +0x9FD4, high confidence: confirmed via SetSpeechFont and
                            // SetNormalFont (both already matched, AC.CPP:13463-13472):
                            // "if ((fontnum<0) || (fontnum>=game.numfonts)) quit(...);" --
                            // disasm's "cmp fontnum,0; jl <quit>; cmp eax,dword_51D2EC; jl <ok>"
                            // matches exactly in both functions. Matches 2011's `int32 numfonts`
                            // (acroom.h:2827) with zero type drift. Sits exactly 8 bytes after
                            // numdialog's start (+0x9FCC), i.e. immediately adjacent modulo the
                            // 4-byte numdlgmessage-shaped gap above -- consistent with 2002
                            // keeping numdialog/numdlgmessage/numfonts contiguous, matching
                            // 2011's declared adjacency for once (unlike chars/playercharacter/
                            // numcharacters above).
  int color_depth;                // +0x9FD8, high confidence: confirmed via FadeOut (already
                            // matched, AC.CPP-equivalent "if (game.color_depth > 1) {...}"):
                            // disasm's "cmp dword_51D2F0,1; jle <skip>" matches exactly, gating a
                            // hi-color-specific fade path (followed by bitmap_color_depth calls).
                            // This whole block of 5 fields (color_depth through hotdotouter) was
                            // found via a remarkable exact-fit: uniqueid (confirmed independently
                            // below) sits EXACTLY 16 bytes after numfonts ends, and 16 bytes is
                            // precisely color_depth(4)+target_win(4)+dialog_bullet(4)+hotdot(2)+
                            // hotdotouter(2) per 2011's declared order (acroom.h:2828-2832) --
                            // zero drift, zero slack. Matches 2011's `int32 color_depth` with
                            // zero type drift.
  int target_win;                 // +0x9FDC, MEDIUM confidence: positional only -- this field
                            // has ZERO references anywhere in the disassembly (plausible for a
                            // build/editor-time-only flag the runtime engine never checks), so
                            // unlike its four neighbors it could not be independently confirmed
                            // via an access site. Included because it's boxed in with zero slack
                            // by the four fields confirmed on either side (see color_depth above)
                            // -- the arithmetic leaves no room for it to be anything else, but
                            // this is weaker evidence than a real usage site.
  int dialog_bullet;              // +0x9FE0, high confidence: confirmed via sub_41D7F7 (called
                            // from do_conversation, already matched, not yet itself named):
                            // "cmp dword_51D2F8,0; jle <skip>; ...spriteset[dword_51D2F8]..."
                            // matches 2011's documented semantics exactly ("0 for none, otherwise
                            // slot num of bullet point", acroom.h:2830) -- used directly as a
                            // sprite-cache index via the already-confirmed SpriteCache::
                            // operator[]. Zero type drift.
  short hotdot;                   // +0x9FE4, high confidence: confirmed via SetMouseCursor
                            // (already matched, AC.CPP-equivalent inventory-cursor-dot logic):
                            // "movsx ecx,word_51D2FC; test ecx,ecx; jle <skip>; cmp arg_0,4;
                            // jnz <skip>" -- gates hotdot-marker drawing specifically for
                            // MODE_USE(4), matching 2011's "hotdot, hotdotouter; // inv cursor
                            // hotspot dot" (acroom.h:2831) exactly. Zero type drift (unsigned
                            // short in 2011; kept as short here per the project's general
                            // preference for signed types unless sign matters).
  short hotdotouter;              // +0x9FE6, high confidence: confirmed the same way as hotdot,
                            // immediately following it in the SAME SetMouseCursor block --
                            // "movsx edx,word_51D2FE; test edx,edx; jle <skip>" gates a second,
                            // "outer ring" color lookup (get_col8_lookup) right after the hotdot
                            // block. Zero type drift.
  int uniqueid;                   // +0x9FE8, high confidence: confirmed via init_translation
                            // (already matched): "if ((uidfrom != game.uniqueid) || (strcmp(
                            // wasgamename, game.gamename) != 0)) { ...quit... }" -- disasm's
                            // "cmp eax,dword_51D300; jnz <fail>; push offset game_gamename;
                            // ...strcmp..." matches exactly, doubly confirming both uniqueid AND
                            // re-confirming gamename's own identity via the same instruction
                            // sequence. Zero type drift.
  int reserved[2];                 // +0x9FEC..0x9FF4 (8 bytes), medium confidence: no access-site
                            // evidence of its own (matches 2011's `int reserved[2]` at
                            // acroom.h:2795 -- reserved fields are rarely referenced by design),
                            // but boxed in with zero slack: it's the ONLY way the exact-fit
                            // arithmetic below closes (see numlang/langcodes/messages/fontflags/
                            // fontoutline's combined comment for the full derivation).
  short numlang;                   // +0x9FF4, medium confidence: no access-site evidence (2011's
                            // own source has zero live references to `numlang` either --
                            // `Engine/AC.CPP:26230`'s `init_language_text(game.langcodes[0])`
                            // call is ITSELF commented out, i.e. dead code even in 2011, matching
                            // the same "system predates or was superseded before 2011" pattern
                            // as `run_event_block`). Matches 2011's `short numlang`
                            // (acroom.h:2796) positionally.
  char langcodes[5][3];            // +0x9FF6..0xA005 (15 bytes), medium confidence: same
                            // evidence status as numlang (MAXLANGUAGE=5, acroom.h:2704). Matches
                            // 2011's `char langcodes[MAXLANGUAGE][3]` (acroom.h:2797) exactly.
  char _pad_align3[3];             // +0xA005..0xA008, compiler alignment padding (not a real
                            // field) -- `messages[500]` below is a pointer array and needs
                            // 4-byte alignment; `langcodes`'s 15-byte extent lands 3 bytes short.
  void *messages[500];             // +0xA008..0xA7D8 (2000 bytes), HIGH confidence: `dword_51D320`.
                            // Confirmed via load_ac2game_dta (already matched): "for(i=0;
                            // i<0x1F4(500);i++) { if(dword_51D320[i*4]==0) continue; else {
                            // malloc(0x1F4=500); dword_51D320[i*4]=result; fread(...) into it
                            // via sub_403024 } }" -- a per-slot conditional message-string loader,
                            // immediately followed (right after this loop ends) by a chain of
                            // `set_default_glmsg` calls for the 12 built-in messages
                            // (MSG_RESTORE=984 etc., already matched, confirms this is genuinely
                            // the global-messages-override system) -- an exact match to 2011's
                            // `char *messages[MAXGLOBALMES=500]` (acroom.h:2799) in both type
                            // (pointer array) and count, zero drift. This is the anchor that
                            // makes the WHOLE surrounding fit trustworthy: its own address is
                            // independently confirmed via real disassembly evidence (not just
                            // arithmetic), and it lands EXACTLY where reserved[2]+numlang+
                            // langcodes+alignment predict, which in turn is EXACTLY 0x800 (2048)
                            // bytes before the already-confirmed numgui once fontflags/
                            // fontoutline (below) are added -- an over-determined fit, stronger
                            // than a simple two-endpoint arithmetic argument.
  unsigned char fontflags[10];    // +0xA7D8..0xA7E2 (10 bytes), medium confidence: no access-site
                            // evidence in THIS build (2011's own `fontflags`-reading code, `AC.CPP
                            // :11655`, lives in a much later, structurally divergent game-loading
                            // routine that also reads `guid`/`saveGameFileExtension`/
                            // `saveGameFolderName` -- all confirmed-later additions with no
                            // counterpart anywhere in this build -- so no useful 2011 anchor
                            // exists for finding this build's own access site, if one exists at
                            // all). Matches 2011's `unsigned char fontflags[10]`
                            // (acroom.h:2803) positionally, boxed in with zero slack.
  char fontoutline[10];           // +0xA7E2..0xA7EC (10 bytes), same evidence status as
                            // fontflags immediately above. Matches 2011's `char fontoutline[10]`
                            // (acroom.h:2804) positionally, boxed in with zero slack -- ends
                            // EXACTLY at the already-confirmed `numgui`'s start.
  int numgui;                     // +0xA7EC, high confidence: confirmed via InterfaceOn AND
                            // InterfaceOff (both already matched, AC.CPP:18596-18619), each with
                            // the identical bitwise-OR range check: "if ((ifn<0) | (ifn>=
                            // game.numgui)) quit(...)". Disasm's "cmp ifn,0; setl al; cmp
                            // ecx,dword_51DB04; setnl dl; or eax,edx" matches both functions
                            // exactly -- same compiler idiom as numcharacters/numdialog above.
                            // Matches 2011's `int32 numgui` (acroom.h:2833) with zero type
                            // drift. NOTE: `numcursors` (2011's very next field, acroom.h:2834)
                            // could NOT be similarly confirmed this round -- every cursor-related
                            // validation/loop in this build (ChangeCursorGraphic,
                            // ChangeCursorHotspot, and a cursor-scaling loop in main) checks
                            // against a HARDCODED `10`, never a runtime field. This looks like a
                            // genuine 2002-vs-2011 behavioral difference (a fixed MAX_CURSOR-style
                            // constant, not yet a runtime-configurable count), not just a
                            // struct-layout question -- see struct-layout-drift.md.
  void *dict;                    // +0xA7F0, high confidence: `WordsDictionary *dict`.
                            // `dword_51DB08`. Confirmed via load_ac2game_dta (already matched):
                            // right after the bulk struct fread, "cmp dword_51DB08,0; jz skip;
                            // malloc(0xBB84); dword_51DB08=result; call sub_4039AB(stream,result)"
                            // -- sub_4039AB reads an int (num_words) into [buf+0], then loops
                            // num_words times reading a 30-byte string per entry (matching
                            // MAX_PARSER_WORD_LENGTH=30, Common/acroom.h:337) into [buf+4+i*30],
                            // plus a 2-byte short into a SECOND fixed-offset array
                            // [buf+0xAFCC+i*2] -- an exact structural match to
                            // WordsDictionary's num_words/word[]/wordnum[] fields (acroom.h:341-
                            // 344), DRIFT: 2011 uses two separate dynamically-sized allocations
                            // (`char **word`, `short *wordnum`); this build flattens both into one
                            // fixed-capacity malloc'd blob referenced by a single pointer -- the
                            // field itself doubles as an on-disk presence flag (0/1) that gets
                            // overwritten with the real pointer once allocated, the same idiom as
                            // compiled_script above. Sits immediately after numgui with zero gap,
                            // matching OriGameSetupStruct2's declared adjacency "int numgui;
                            // WordsDictionary *dict;" (acroom.h:2805-2806) exactly.
  char _pad_unknown7[0x1790];    // +0xA7F4..0xBF84, unknown (6032 bytes) -- everything declared
                            // in `OriGameSetupStruct`/`OriGameSetupStruct2` is now accounted for
                            // above this point, so this trailing pad likely contains ONLY
                            // whatever fixed-size arrays the further-derived `GameSetupStruct`
                            // embeds directly (`spriteflags`, `invinfo[100]`, etc.,
                            // acroom.h:2890-2917) -- unconfirmed. `numcursors`,
                            // `default_lipsync_frame`,
                            // `invhotdotsprite`, AND `default_resolution` are each believed
                            // ABSENT from this build entirely (not merely unfound) -- see
                            // struct-layout-drift.md's writeups on each (every runtime check for
                            // numcursors is hardcoded to 10; SetMouseCursor's confirmed body has
                            // no invhotdotsprite-sprite branch; GetLipSyncFrame's two most
                            // distinctive calls, strnicmp and strchr('/'), have zero matching
                            // candidates anywhere in the disassembly; `default_resolution`'s
                            // consuming code in `main` -- Engine/AC.CPP:27739-27782's whole
                            // multi-branch native-resolution selector -- has NO counterpart at
                            // all in this build, which instead hardcodes scrnwid/scrnhit=320x200
                            // then branches purely on `usetup_screenres`, a PLAYER/config setting
                            // (0/1/2 -> 320x200/640x400/960x600), not any `game.*` field --
                            // confirmed absent two more ways: `OriGameSetupStruct` never declares
                            // it at all, and 2011's own `ConvertOldGameStruct` upgrade path
                            // (acroom.h:3017-3051) leaves it unset when upgrading an old-format
                            // game -- unlike `numcursors`, which that same function explicitly
                            // hardcodes to 10 as a fallback. The whole "author declares a native
                            // game resolution, engine scales to fit" feature is a later addition;
                            // this build's resolution is purely a player-side window-scale choice).
  //
  // MAJOR FINDING (see struct-layout-drift.md for the full writeup): this struct is not a
  // shrunk/drifted GameSetupStructBase at all -- it is `OriGameSetupStruct` (Common/
  // acroom.h:2769-2800), the OLDEST ancestor in AGS's own struct-evolution chain
  // (OriGameSetupStruct -> OriGameSetupStruct2 -> OldGameSetupStruct -> ... ->
  // GameSetupStructBase), preserved read-only in the 2011 header purely for old-save-file
  // compatibility (see ConvertOldGameStruct, acroom.h:3017). This retroactively explains every
  // "drastic drift" finding in this struct's recovery (byte-sized options[20] vs int32[100],
  // gamename[30] vs [50], the chars/playercharacter field-order divergence) as this build simply
  // predating all of GameSetupStructBase's later evolution, not scattered independent drift.
  // globalscript/numcharacters/chars and numgui/dict were confirmed via exact zero-slack
  // adjacency matches to OriGameSetupStruct's/OriGameSetupStruct2's own declared field order.
  //
  // OFFSET CORRECTION (applied before this ever reached the live IDB): every field from
  // `globalscript` onward was ORIGINALLY computed +8 bytes too low, because the byte-offset-
  // counting technique used to find them (see count_data_offsets.py) round-tripped `align 10h`
  // through the running RELATIVE offset from `game_gamename` instead of the true ABSOLUTE
  // address -- and `game_gamename`'s real base (`0x513318`) is not itself 16-byte aligned
  // (`0x513318 mod 0x10 == 8`), so the two calculations diverge by exactly 8 bytes at that one
  // `align 10h` (immediately before the `mcurs[10]`-shaped region, since confirmed -- see below)
  // and every offset after it. Caught by cross-checking the script's output against DIRECT hex-address
  // subtraction for every label whose IDA name embeds its own real address (`dword_51594C`,
  // `dword_515950`, `dword_51B010`, `dword_51B84C`, `dword_51D2E4`...`dword_51DB08`) -- all 14
  // direct computations disagreed with the (buggy) script by the same +8, and all 14 agreed
  // with each other and with the corrected script (which now takes the true base address as a
  // parameter and aligns the absolute address, not the relative offset) to the byte. This
  // affected every field below `numviews` in this struct (globalscript through dict) but NOT
  // `numviews` itself or anything above it (both align directives before `numviews` -- one at
  // `align 4`, one earlier -- happen not to matter: the base is already a multiple of 4, so
  // only the later `align 10h` exposed the bug). Fixed before ever being applied to the live
  // IDB via this script -- if this struct was already applied to your IDB from an EARLIER
  // export of this file, re-run apply_structs.py to correct it.
  //
  // Confirmed fields so far span +0x00 to +0xA7F4 (42996 bytes) out of the known total 0xBF84
  // (49028 bytes) -- every field `OriGameSetupStruct`/`OriGameSetupStruct2` declares is now
  // accounted for. Remaining unrecovered content is entirely in the trailing `_pad_unknown7`
  // gap: whatever fixed-size arrays the further-derived `GameSetupStruct` embeds directly
  // (`invinfo[100]`, `spriteflags`, acroom.h:2890-2917) that this build predates
  // `OriGameSetupStruct`'s own layout for.
  //
  // CRITICAL: unlike GUIButton/GUITextBox/etc. (dynamically allocated, no independently-known
  // total size to preserve), this struct's global instance already had a CONFIRMED exact size
  // (0xBF84, from the live IDB, predating this recovery) before any of the above fields were
  // named. The padding fields above are NOT optional -- omitting any would shrink the applied
  // type below its known-correct 0xBF84, corrupting the type for the fixed global instance.
  // Every future partial extension of this struct MUST keep the total at 0xBF84.
};

struct ExecutingScript {
  void *inst;                     // +0x00, high confidence: `ccInstance *inst`. Confirmed via
                            // post_script_cleanup (already matched): read with NO added offset
                            // from the scripts[] array (dword_4CC848[(num_scripts-1)*0x6C]) and
                            // passed straight to the already-matched ccFreeInstance. Matches
                            // 2011's FIRST declared field (Common/acruntim.h:701) exactly.
                            // Cross-confirmed by ExecutingScript::init (sub_424A00), which
                            // zeroes this offset first.
  int newnum;                     // +0x04, high confidence: sentinel -1 = "no pending room
                            // change". IDA's own pre-existing local-variable name for this
                            // offset (decoded from post_script_cleanup's bulk-copy buffer).
                            // Confirmed via usage: "cmp [newnum],0; jl skip; ...call
                            // new_room(newnum, playerchar)" if no scripts still running, else
                            // deferred onto the NEXT script slot ("dword_52314C[+4]=newnum").
                            // Matches 2011's ePSANewRoom PostScriptAction case
                            // (Common/acruntim.h:687). Cross-confirmed by
                            // ExecutingScript::init, which sets this to -1.
  int invscreen;                  // +0x08, high confidence: boolean, 0 = "no pending inventory
                            // screen". Confirmed via "cmp [invscreen],0; jz skip; call
                            // sub_41FEA9" where sub_41FEA9 (not independently renamed, a
                            // single-purpose gate) unconditionally calls the already-matched
                            // __actual_invscreen(). Matches 2011's ePSAInvScreen case
                            // (acruntim.h:688). Cross-confirmed by ExecutingScript::init,
                            // which sets this to 0.
  int ooo;                        // +0x0C, high confidence: pending restore-game slot number.
                            // IDA's own pre-existing local-variable name (kept as-is: "ooo" is
                            // IDA's generic auto-name, but its position/usage is independently
                            // confirmed, unlike coincidental "ooo"/"var_8" name reuse seen in
                            // other, unrelated functions -- always verify by usage, never by
                            // name alone). Confirmed via "cmp [ooo],0x3E8(1000); jnz notdlg;
                            // call RestoreGameDialog; jmp done; notdlg: cmp [ooo],0; jl done;
                            // call sub_409A9C (frees/resets all currently-executing scripts);
                            // push 0 (nametouse); push [ooo]; call restore_game_data(ooo,
                            // nametouse)". Matches 2011's ePSARestoreGame/ePSARestoreGameDialog
                            // PAIR (acruntim.h:689-690) folded into ONE field plus a magic
                            // sentinel (1000="show dialog"; -1="none"; else=direct slot
                            // number) -- 2011 keeps these as two separate enum cases. Cross-
                            // confirmed by ExecutingScript::init, which sets this to -1.
  int dlgnum;                     // +0x10, high confidence: sentinel -1 = "no pending dialog".
                            // IDA's own pre-existing local-variable name. Confirmed via "cmp
                            // [dlgnum],0; jl skip; push dlgnum; call do_conversation(dlgnum)".
                            // Matches 2011's ePSARunDialog case (acruntim.h:691). Cross-
                            // confirmed by ExecutingScript::init, which sets this to -1.
  char script_run_another[2][30]; // +0x14..0x50 (60 bytes), high confidence: DRIFT -- capacity
                            // is 2 here (confirmed via `cmp [this+0x60],2` in
                            // ExecutingScript::run_another), not 2011's declared
                            // MAX_QUEUED_SCRIPTS=4 (acruntim.h:698). Inner dimension (30 bytes/
                            // entry) matches acruntim.h:707 exactly, zero drift. See
                            // ExecutingScript::run_another (sub_425500) in matches.json.
  int run_another_p1[2];          // +0x50..0x58 (8 bytes), high confidence, see run_another.
  int run_another_p2[2];          // +0x58..0x60 (8 bytes), high confidence, see run_another.
  int numanother;                 // +0x60, high confidence: count of queued run_another
                            // entries (0..2). See ExecutingScript::run_another. Cross-
                            // confirmed by ExecutingScript::init, which sets this to 0.
  int restartgame;                // +0x64, high confidence: boolean, 0 = "no pending restart".
                            // IDA's own pre-existing local-variable name for this offset
                            // (decoded the same way as newnum/ooo/dlgnum). Confirmed via, at
                            // the very end of post_script_cleanup: "cmp [restartgame],0; jz
                            // done; call sub_409A9C; call RestartGame". Matches 2011's
                            // ePSARestartGame case (acruntim.h:693). Cross-confirmed by
                            // ExecutingScript::init, which sets this to 0.
  char forked;                    // +0x68, high confidence: confirmed via the SAME
                            // post_script_cleanup call site -- byte_4CC8B0[(num_scripts-1)*0x6C]
                            // (0x4CC8B0-0x4CC848=0x68 bytes into the same array stride) gates
                            // the ccFreeInstance call as a boolean. Matches 2011's LAST declared
                            // field (`char forked;`, acruntim.h:711) in BOTH position (last field)
                            // and semantics (forked-instance cleanup gate) exactly. Cross-
                            // confirmed by ExecutingScript::init, which zeroes this offset last.
  char _pad_align2[3];           // +0x69..0x6C, likely compiler alignment padding (not confirmed
                            // as a real field) -- total stride independently confirmed at 0x6C
                            // (108 bytes) via a `rep movsd` bulk-copy of exactly 0x1B (27)
                            // dwords in post_script_cleanup, matching this struct's declared
                            // total exactly.
  // STRUCT FULLY MAPPED: zero unaccounted bytes from +0x00 to +0x6C (108 bytes), cross-
  // confirmed by ExecutingScript::init (sub_424A00) independently touching exactly these 8
  // offsets in exactly this order with matching sentinels. DRASTIC DRIFT from 2011: the full
  // ~725-byte struct (dominated by the postScriptActions[MAX_QUEUED_ACTIONS=5] queue system,
  // including a 500-byte postScriptSaveSlotDescription[5][100]) does not exist in this 2002
  // build -- ARCHITECTURAL FINDING: 2002 gives 5 of 2011's 9 PostScriptAction cases (all but
  // SaveGame/SaveGameDialog/RunAGSGame) their OWN dedicated field; the generic queue is a
  // LATER unification, not a reduced version of something already present here.
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
        print("OK: applied verified-safe type declarations (block, GUIMain, CharacterInfo, "
              "ccInstance, ccScript, GUIButton, GUITextBox, GUILabel, GUIListBox, GUIInv, "
              "GUISlider, SpriteCache, EventBlock, MouseCursor, InterfaceElement, "
              "GameSetupStructBase, ExecutingScript).")
    else:
        print(f"parse_decls reported {err_count} error(s) -- check the declarations above.")


if __name__ == "__main__":
    main()
