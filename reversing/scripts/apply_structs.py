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
  - `GUIMain`: mostly filled in now. Unlike most other structs in this file,
    the core fields (x/y/numobjs/mouseover/mousedownon/on/objs/objrefptr)
    were originally read directly off real disassembly instructions, not
    copied from 2011 -- see reversing/notes/struct-layout-drift.md for the
    full derivation. A later round found that EVERY one of those
    independently-confirmed fields lands exactly where 2011's CURRENT/MODERN
    `GUIMain` declaration (`Common/acgui.h:664-687`) predicts -- unlike most
    other structs in this project, which match an OLD save-compat ancestor
    instead, this one matches 2011's live layout with zero drift (a rare
    case, alongside `MouseCursor`/`InventoryItemInfo`). The remaining fields
    are filled in from that same declaration, MEDIUM confidence unless
    individually confirmed (see each field's own comment) -- an
    "over-determined fit": five separate previously-opaque gaps ALL close
    with zero slack simultaneously against one internally-consistent 2011
    declaration, which is strong corroborating evidence even where a field
    lacks its own direct access-site confirmation. Total size (0x184,
    confirmed via an `imul reg, 184h` array-index computation in
    remove_popup_interface) is exact, and matches 2011's own declared
    save-file size (`27 + 2*MAX_OBJS_ON_GUI` ints after a 40-byte header,
    computed in `GUIMain::ReadFromFile`) exactly -- notably NOT including
    `drawOrder[MAX_OBJS_ON_GUI]`, which 2011 explicitly regenerates rather
    than persists, consistent with this build's confirmed total excluding
    it too.

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

  - `GameSetupStructBase`: **FULLY MAPPED** -- every byte from `+0x00`
    through `+0xBF84` (49028 bytes total) is now accounted for, the
    largest and longest-running struct-recovery effort in this project
    (2011's own `GameSetupStructBase` is only ~3900 bytes; this build's
    version, matching `OriGameSetupStruct`'s much older, flatter layout,
    is over 12x bigger). 37 fields confirmed: `gamename`, `options`,
    `paluses`, `defpal[256]`, `iface[10]`, `numiface`, `numviews`,
    `mcurs[10]`, `globalscript`, `numcharacters`, `chars`,
    `__charcond[50]`, `__invcond[100]`, `compiled_script`,
    `playercharacter`, `totalscore`, `numinvitems`, `invinfo[100]`,
    `numdialog`, `numdlgmessage`, `numfonts`, `color_depth`,
    `target_win`, `dialog_bullet`, `hotdot`, `hotdotouter`, `uniqueid`,
    `reserved[2]`, `numlang`, `langcodes[5][3]`, `messages[500]`,
    `fontflags[10]`, `fontoutline[10]`, `numgui`, `dict`,
    `reserved2[8]`, `spriteflags[6000]` -- plus 4 fields independently
    CONFIRMED ABSENT (`numcursors`, `default_lipsync_frame`,
    `invhotdotsprite`, `default_resolution` -- each a genuine, later
    AGS addition this 2002 build predates, not merely unfound; see
    struct-layout-drift.md for the individual absence proofs). Every
    field's own comment below carries its full evidence; see
    struct-layout-drift.md for the complete round-by-round history of
    how this struct was cracked open (dozens of rounds, several genuine
    dead ends and retractions along the way, all documented there
    rather than repeated here).
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
  // Fields below +0x28 (x) are ALL independently confirmed (see each field's own history in
  // struct-layout-drift.md); everything ELSE in this struct is filled in from 2011's CURRENT
  // `GUIMain` declaration (`Common/acgui.h:664-687`), which those confirmed fields match with
  // zero drift -- an over-determined fit across five separate previously-opaque gaps. MEDIUM
  // confidence unless a field has its own direct access-site note below.
  char vtext[4];             // +0x00, MEDIUM confidence: positional/arithmetic fit only. 2011's
                           // own comment calls this "for compatibility" -- likely already vestigial
                           // even in 2011, plausibly more so here.
  char name[16];              // +0x04, MEDIUM confidence: positional/arithmetic fit only, matching
                           // 2011's declared `char name[16]; // the name of the GUI`.
  char clickEventHandler[20]; // +0x14, MEDIUM confidence: positional/arithmetic fit only, matching
                           // 2011's declared field.
  int x;                  // +0x28, confirmed via GUIMain::mouse_but_down; RECONFIRMED via
                           // `GetGUIAt` (already matched, script-exported) as part of its
                           // point-in-bounding-box hit test.
  int y;                  // +0x2C, confirmed via GUIMain::mouse_but_down; RECONFIRMED via
                           // `GetGUIAt` the same way as `x` above.
  int wid;                  // +0x30, high confidence (UPGRADED from MEDIUM): confirmed via
                           // `GetGUIAt` (already matched, script-exported): "eax=guis[idx].x;
                           // add eax,[guis+idx*184h+30h]; cmp xx,eax; setle cl" -- computes
                           // `x+wid` as the right edge of the bounding-box hit test, matching
                           // 2011's "xx<=guis[aa].x+guis[aa].wid" exactly. Matches 2011's
                           // declared field (`acgui.h:669`) in position exactly.
  int hit;                  // +0x34, high confidence (UPGRADED from MEDIUM): confirmed the same
                           // way as `wid` immediately above -- `GetGUIAt` computes `y+hit` as
                           // the bottom edge of the bounding-box hit test, matching 2011's
                           // "yy<=guis[aa].y+guis[aa].hit" exactly.
  int focus;                 // +0x38, MEDIUM confidence: positional/arithmetic fit only, matching
                           // 2011's declared "which object has the focus" field.
  int numobjs;             // +0x3C, high confidence: confirmed via GUIMain::get_control_type's bounds
                           // check ("if (indx<0 || indx>=numobjs) return -1;").
  int popup;                 // +0x40, high confidence (UPGRADED from MEDIUM): confirmed via
                           // `check_controls` (already matched): "cmp [guis+ev2*184h+40h],1; jz
                           // <continue-popup-mouseover-check>" -- a literal exact-match check
                           // against `1`, matching 2011's declared `POPUP_MOUSEY=1` (`Common/
                           // acroom.h:277`) precisely (gating the "should this GUI auto-show when
                           // the mouse nears the trigger line" logic). Matches 2011's declared
                           // field (`acgui.h:672`) in position and semantic role exactly.
  int popupyp;                // +0x44, high confidence (UPGRADED from MEDIUM): confirmed via BOTH
                           // `check_controls` ("mov eax,mouseY; cmp eax,[guis+ev2*184h+44h]; jge
                           // <not-yet>" -- guarding the "show this popup GUI" trigger, only
                           // proceeding while `mouseY < popupyp`) and `remove_popup_interface`
                           // (already matched -- whose own matches.json evidence already cited
                           // this offset as "popupyp-related" from an earlier round, before this
                           // struct-field round propagated it into apply_structs.py): "if
                           // (mouseY<=guis[ifn].popupyp) msetgraphpos(mouseX, popupyp+2);" -- the
                           // "reposition the mouse when auto-hiding the popup" logic. Matches
                           // 2011's declared field (`acgui.h:673`, "popup when mousey < this") in
                           // position and semantic role exactly.
  int bgcol;                  // +0x48, MEDIUM confidence: positional/arithmetic fit only, boxed in
                           // with zero slack between the confirmed `popupyp` and `bgpic` fields.
  int bgpic;                  // +0x4C, high confidence: confirmed via `SetGUIBackgroundPic` (already
                           // matched, script-exported): "mov [guis+guin*184h+4Ch], slotn" -- sets
                           // this field directly from its `slotn` parameter, matching 2011's
                           // declared field in position exactly.
  int fgcol;                  // +0x50, MEDIUM confidence: positional/arithmetic fit only, boxed in
                           // with zero slack between the confirmed `bgpic` and `mouseover` fields.
  int mouseover;          // +0x54, confirmed via GUIMain::mouse_but_down
  int mousewasx;              // +0x58, MEDIUM confidence: positional/arithmetic fit only, matching
                           // 2011's declared `mousewasx, mousewasy` adjacency.
  int mousewasy;              // +0x5C, MEDIUM confidence: same status as `mousewasx` immediately above.
  int mousedownon;        // +0x60, confirmed via GUIMain::mouse_but_up/down
  int highlightobj;           // +0x64, MEDIUM confidence: positional/arithmetic fit only, matching
                           // 2011's declared field.
  int flags;                  // +0x68, high confidence (UPGRADED from MEDIUM): confirmed via
                           // `GetGUIAt` (already matched, script-exported): "edx=guis[idx].flags;
                           // and edx,1; test edx,edx; jz <continue-hit-test>; else <skip-this-gui>"
                           // -- a bit-0 check gating the hit test, matching 2011's declared
                           // `GUIF_NOCLICK=1` (`acgui.h:662`) exactly ("if (guis[aa].flags &
                           // GUIF_NOCLICK) continue;"). `SetGUIClickable` itself (the function that
                           // would SET this bit) was searched for and NOT found anywhere in this
                           // binary -- consistent with this project's repeated "later API, not yet
                           // added" pattern -- but the bit is genuinely READ and matches exactly.
  int transparency;           // +0x6C, MEDIUM confidence: positional/arithmetic fit only. Checked
                           // for `SetGUITransparency` this round -- also not found in this binary,
                           // same caveat as `flags` above.
  int zorder;                 // +0x70, MEDIUM confidence: positional/arithmetic fit only.
                           // `SetGUIZOrder`/`GUI_SetZOrder`/`update_gui_zorder` -- the whole
                           // 2011 z-order machinery -- were searched for and NOT found anywhere
                           // in this binary. Stronger supporting evidence than a mere absent
                           // name/string: `GetGUIAt` (already matched) iterates GUIs by RAW INDEX
                           // directly (`var_4` counting down from `numgui-1` to `0`, used straight
                           // as the array multiplier) with NO `play.gui_draw_order[var_4]`
                           // indirection at all -- unlike 2011's "aa = play.gui_draw_order[ll];"
                           // (`Engine/AC.CPP:16090`). This build most likely predates GUI z-order
                           // sorting as a feature entirely; `zorder` itself may still exist as an
                           // inert field (matching the struct's own total-size arithmetic), just
                           // not yet wired up to anything that reads or writes it.
  int guiId;                  // +0x74, MEDIUM confidence: positional/arithmetic fit only, matching
                           // 2011's declared field.
  int reserved[6];             // +0x78..0x90 (24 bytes), MEDIUM confidence: positional/arithmetic
                           // fit only, boxed in with zero slack between the confirmed `guiId` and
                           // `on` fields -- plausible for genuinely unused reserved space, matching
                           // the field's own name/intent (same reasoning already used for
                           // `InventoryItemInfo.reserved`).
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
                           //
                           // Everything from here through the end of the struct matches 2011's
                           // OLD `OldCharacterInfo` ancestor (`Common/acroom.h:2599-2621`) --
                           // NOT the modern `CharacterInfo` (`acroom.h:2504`) -- position-for-
                           // position with ZERO drift on every field checked so far, including
                           // the struct's own total size (0x140, matching exactly). This mirrors
                           // the `GameSetupStructBase`=`OriGameSetupStruct` finding: this build's
                           // struct is a still-live implementation of what 2011 keeps only as a
                           // save-compat ancestor declaration.
  short following;          // +0x24, high confidence: `FollowCharacterEx` (already matched,
                           // script-exported): "mov dx,word[tofollow]; mov [chars+who*140h+
                           // 24h],dx" -- sets this field directly from its `tofollow` parameter.
                           // Matches 2011's declared field (`acroom.h:2606`) exactly.
  short followinfo;         // +0x26, high confidence: `FollowCharacterEx` (same call, immediately
                           // after `following`): "eax=(distaway<<8)|eagerness; mov [chars+who*
                           // 140h+26h],ax" -- a packed byte pair (distance in the high byte,
                           // eagerness in the low byte), matching 2011's declared field
                           // (`acroom.h:2607`) in position and the same packed-byte convention.
  int idleview;              // +0x28, high confidence: `SetCharacterIdle` (already matched,
                           // script-exported): "mov [chars+who*140h+28h], iview-1" -- sets this
                           // field to the caller's `iview` parameter minus 1. Matches 2011's
                           // declared field (`acroom.h:2608`) in position and type exactly.
  short idletime;           // +0x2C, high confidence: walk_character does
                           // "chin->idleleft=chin->idletime;" -- disasm matches verbatim:
                           // [+0x2E] = [+0x2C] inside the exact "if (chin->idleleft < 0)" branch.
                           // ALSO reconfirmed via `SetCharacterIdle` (already matched): sets both
                           // `idletime`@+0x2C and `idleleft`@+0x2E directly from its `itime`
                           // parameter, matching source's "chin->idletime=itime; chin->idleleft=
                           // itime;" exactly.
  short idleleft;           // +0x2E, high confidence: same evidence as idletime -- walk_character's
                           // "if (chin->idleleft < 0) { ReleaseCharacterView(...); ... }" matches a
                           // signed [+0x2E]<0 check driving a call to the already-matched ReleaseCharacterView.
                           // Also independently re-confirmed via Character_UnlockView's own
                           // "chaa->idleleft = chaa->idletime;" at its end, and via SetCharacterIdle
                           // (see idletime above).
  short transparency;       // +0x30, high confidence: `SetCharacterTransparency` (already matched,
                           // script-exported): "if(trans==0) t=0; else if(trans==100) t=0xFF; else
                           // t=((100-trans)*25)/10; mov [chars+obn*140h+30h], t" -- the EXACT same
                           // transparency-percentage-to-byte formula already confirmed for
                           // `RoomObject.transparent`. Matches 2011's declared field
                           // (`acroom.h:2610`) in position and semantic role exactly.
  short baseline;            // +0x32, high confidence: `SetCharacterBaseline` (already matched,
                           // script-exported): "mov ax,word[basel]; mov [chars+obn*140h+32h],ax"
                           // -- sets this field directly from its `basel` parameter. Matches
                           // 2011's declared field (`acroom.h:2611`) in position exactly.
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
  short inv[100];            // +0x44..0x10C (200 bytes), high confidence: CORRECTS the earlier
                           // pre-existing "int inv" annotation -- `main` (already matched): "mov
                           // word ptr [playerchar+ee*2+44h], 1" (or 0) sets each item's starting-
                           // inventory count as a 2-byte write inside a `for(ee=0;ee<numinvitems;
                           // ee++)` loop, matching 2011's OLD ancestor `short inv[100]`
                           // (`acroom.h:2616`) exactly in both type (short, not int) and position.
                           // Only element-level access confirmed (the array's own existence/type/
                           // start position is HIGH confidence; individual elements beyond the
                           // one directly observed are inferred from the confirmed stride, same
                           // standard used for every other array in this project).
  short actx;               // +0x10C, MEDIUM confidence: NOT independently confirmed via its own
                           // access site -- positional inference only, matching 2011's declared
                           // field (`acroom.h:2617`) immediately after `inv[100]`. Checked and NOT
                           // found this round: 2011's only usage site for `actx`/`acty`
                           // (`Engine/AC.CPP:8525-8526`) sits deep inside hardware-accelerated
                           // drawing code (`gfxDriver`/`actspsbmp`/`SetTint`/`SetLightLevel`) that
                           // this build has already been shown, repeatedly, to predate entirely --
                           // plausibly a genuinely later addition, not just unfound.
  short acty;                // +0x10E, MEDIUM confidence: same status as `actx` immediately above.
  char name[30];              // +0x110..0x12E, high confidence (UPGRADED from MEDIUM): confirmed
                           // via `GetLocationName` (already matched, script-exported): "lea
                           // ecx,[game_chars+idx*140h+110h]; push ecx; call GetTranslation" --
                           // passes this field straight to the already-matched `GetTranslation`
                           // and `strcpy`'s the result into the caller's buffer, matching 2011's
                           // "strcpy(tempo,get_translation(game.chars[onhs].name));" exactly.
                           // Matches 2011's declared field (`acroom.h:2618`) in position exactly.
  char scrname[16];           // +0x12E..0x13E, high confidence (UPGRADED from MEDIUM): confirmed
                           // via `compile_room_script` (already matched): "movsx edx,byte[game_chars
                           // +aa*140h+12Eh]; test edx,edx; jz <skip>" (checking `scrname[0]==0`)
                           // then "lea edx,[game_chars+aa*140h+12Eh]; push edx; call strcat" --
                           // matching 2011's "if (game.chars[aa].scrname[0]==0) continue; ...
                           // strcat(temphdr,game.chars[aa].scrname);" exactly (building "#define
                           // cEgo 0\r\n"-style macros mapping script names to character indices
                           // for room-script compilation). Matches 2011's declared field
                           // (`acroom.h:2619`) in position exactly.
  char on;                   // +0x13E, high confidence (UPGRADED from MEDIUM): confirmed via
                           // `sub_417ECD` (an unnamed internal helper, called from `GetCharacterAt`,
                           // already matched): "movsx ecx,byte[chars+idx*140h+13Eh]; test ecx,ecx;
                           // jz <skip-this-character>" -- part of a per-character filter loop
                           // (room match, `on`, `!(flags & CHF_NOINTERACT)`, valid `view`) used
                           // to find which character is at a given screen location. Matches
                           // 2011's declared LAST field (`acroom.h:2620`) in position and
                           // semantic role ("is this character visible/clickable") exactly.
  char _pad_align[1];        // +0x13F..0x140, compiler alignment padding (not a real field) --
                           // boxed in with zero slack by the confirmed total stride (0x140) and
                           // `on`'s own predicted end at +0x13F.
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
  void *exportaddr[600];      // +0x1C..0x97C, high confidence (RESOLVES the region formerly tracked
                            // as an unknown 2400-byte pad). ccCreateInstanceEx's export-address
                            // resolution loop (source CSRUN.CPP:933-948) writes directly into
                            // `[cinst + idx*4 + 0x1C]` -- an array EMBEDDED inline in the struct,
                            // not a separately malloc'd `char **exportaddr` pointer like 2011's.
                            // Loop bound is the already-confirmed ccScript.numexports@+0x1C48; the
                            // per-entry computation matches source exactly for both export types
                            // (EXPORT_FUNCTION -> &cinst->code[eaddr], EXPORT_DATA -> cinst->
                            // globaldata+eaddr, CSRUN.CPP:940/942), and the auto-import loop right
                            // after reads the same array back (`[cinst+idx*4+0x1Ch]`) as the 3rd arg
                            // to SystemImports::add, matching source's `simp.add(scri->exports[i],
                            // cinst->exportaddr[i], cinst)` (CSRUN.CPP:959) exactly -- three
                            // independent confirmations. Capacity closes with ZERO slack: 600
                            // entries (matching ccScript.export_addr[600]'s own confirmed capacity)
                            // x 4 bytes = 2400 = 0x960, landing exactly on stack@+0x97C. This also
                            // retroactively explains why the interpreter (sub_42B394) never once
                            // touches this region (see its own matches.json entry and reversing/
                            // notes/csrun-interpreter-evolution.md for the earlier, now-superseded
                            // call-stack-array hypothesis disproof) -- exportaddr is populated once
                            // at instance-creation time and never touched by the bytecode loop
                            // itself. CLOSES ccInstance COMPLETELY -- every byte from +0x00 through
                            // the struct's confirmed +0x9A8 total size is now accounted for.
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
  int line_number;            // +0x9A0, high confidence: SCMD_LINENUM (opcode 36, "debug info --
                            // source code line number", Common/CSCOMP.H:303) handler inside the
                            // interpreter (sub_42B394) does `[ecx+9A0h] = edx; dword_5347F4 = edx`
                            // in one place -- matches 2011's `case SCMD_LINENUM: inst->line_number =
                            // arg1; currentline = arg1;` (CSRUN.CPP:1334-1336) exactly, and
                            // independently identifies dword_5347F4 as the global `currentline`.
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
  char *fixuptypes;            // +0x18, high confidence (UPGRADED from a TENTATIVE positional-only
                            // padding guess): confirmed via `ccCreateInstanceEx` (already matched):
                            // its fixup-processing loop does "movsx ecx,byte ptr[[scri+0x18]+i]" --
                            // reading a per-fixup TYPE byte through a POINTER stored at this offset
                            // (matching 2011's declared `char *fixuptypes`, not an embedded array),
                            // then switches on it (1-6) to relocate `code[fixups[i]]` by adding the
                            // new instance's own `globaldata`(+0x04, case 1/FIXUP_GLOBALDATA) or
                            // `strings`(+0x14, case 3/FIXUP_STRING) base address, or resolving a
                            // system import (case 4/FIXUP_IMPORT, via the already-matched
                            // `SystemImports::is_script_import`) -- matching 2011's declared
                            // `FIXUP_*` constants (`Common/CSCOMP.H:165-170`) and switch behavior
                            // exactly. Matches 2011's declared field (`CSCOMP.H:208`) in position
                            // and type exactly.
  long *fixups;                // +0x1C, high confidence (same upgrade/evidence as `fixuptypes`
                            // above): the fixup-loop's other operand, read as "[[scri+0x1C]+i*4]"
                            // through a POINTER at this offset (matching 2011's declared `long
                            // *fixups`, a code-array-index list, not an embedded array). Matches
                            // 2011's declared field (`CSCOMP.H:209`) in position and type exactly.
  int numfixups;                // +0x20, high confidence (same upgrade/evidence as `fixuptypes`
                            // above): the fixup-loop's own bound ("cmp i,[scri+0x20]; jge <done>").
                            // Matches 2011's declared field (`CSCOMP.H:210`) in position exactly.
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

struct InventoryItemInfo {
  // Total size 0x44 (68 bytes), high confidence -- independently confirmed via its own array
  // stride (`imul reg,44h` at every access site below) AND matches 2011's own declared total
  // size (Common/acroom.h:2624-2629: name[25]+pad[3]+pic(4)+cursorPic(4)+hotx(4)+hoty(4)+
  // reserved[5](20)+flags(1)+pad[3] = 68) exactly -- a RARE case of genuinely ZERO drift in
  // every independently-confirmed field's offset, matching MouseCursor's own "best fit" case.
  char _unconfirmed1[0x1C];       // +0x00..0x1C (28 bytes), matches 2011's `char name[25]` plus
                            // 3 bytes of compiler alignment padding by POSITION only -- no
                            // access-site evidence found this round (the item's display name is
                            // presumably read/written by GetInvName/SetInvItemName-equivalent
                            // functions not yet traced).
  int pic;                        // +0x1C, high confidence: `dword_51B870`. Confirmed THREE
                            // independent ways: (1) `SetInvItemPic` (already matched,
                            // script-exported) writes its `piccy` parameter directly here,
                            // matching AC.CPP:5243's `game.invinfo[invItemId].cursorPic = piccy;`
                            // in SPIRIT (this build's SetInvItemPic sets `pic`, not a separate
                            // `cursorPic` -- see the `_unconfirmed2` note below); (2) `GUIInv::Draw`
                            // (already matched) reads it as the sprite-cache index for rendering
                            // the item's inventory icon; (3) `sub_40CF16` (called from
                            // `SetInvItemPic` and `SetMouseCursor`, not itself renamed) reads it
                            // to set the MODE_USE cursor's picture, and again as a sprite-index
                            // for the auto-center-hotspot fallback. Lands at EXACTLY 2011's
                            // declared `pic` offset (right after `name[25]`+padding), zero drift.
  char _unconfirmed2[4];          // +0x20..0x24 (4 bytes), matches 2011's `int cursorPic` by
                            // POSITION only (boxed in with zero slack between the confirmed
                            // `pic` and `hotx` fields, exactly 2011's declared `cursorPic` size
                            // and position) -- no access site found this round that reads a
                            // field distinct from `pic` for cursor-picture purposes; every
                            // cursor-picture code path traced this round (`sub_40CF16`) reads
                            // `pic` directly instead. Plausible explanation: this build's
                            // cursor-picture logic simply doesn't use `cursorPic` yet (the field
                            // may still exist in memory, just unread/unwritten by any traced code
                            // path), not that the field itself is absent -- unlike fields
                            // confirmed genuinely ABSENT elsewhere in this project (e.g.
                            // `numcursors`), there's no NEGATIVE evidence here, just an
                            // unconfirmed gap. Do not assert a name for this field without
                            // independent access-site evidence.
  int hotx;                       // +0x24, high confidence: `dword_51B878`. Confirmed via
                            // `sub_40CF16` (reads it, applies to `mcurs[MODE_USE].hotx`) AND via
                            // `main`'s startup sequence (scales it by
                            // `current_screen_resolution_multiplier_x`) -- both read/write it as
                            // a FULL DWORD, matching 2011's declared `int hotx` (acroom.h:2627)
                            // with ZERO type drift (unlike `MouseCursor`'s own `hotx`, which IS a
                            // `short` in this build -- these are two different, independently-
                            // confirmed types for similarly-named fields in different structs,
                            // don't conflate them). Lands at EXACTLY 2011's declared `hotx`
                            // offset, zero drift.
  int hoty;                       // +0x28, high confidence: `dword_51B87C`. Confirmed the same
                            // two ways as `hotx` immediately above (`sub_40CF16` and `main`'s
                            // resolution-scaling loop). Matches 2011's declared `int hoty`
                            // (acroom.h:2627) with zero drift, at its exact predicted offset.
  char _unconfirmed3[0x14];       // +0x2C..0x40 (20 bytes), matches 2011's `int reserved[5]`
                            // (acroom.h:2628) by POSITION only -- boxed in with zero slack
                            // between the confirmed `hoty` and `flags` fields, no access-site
                            // evidence (plausible for genuinely unused reserved space, matching
                            // the field's own name/intent).
  char flags;                     // +0x40, high confidence: `byte_51B894`. Confirmed via `main`'s
                            // startup sequence: "movsx eax,byte_51B894[ee*0x44]; and eax,1; jz
                            // <skip>; ...playerchar->inv[ee]=1..." -- checking bit 0 and, if set,
                            // giving the player 1 starting copy of the item, matching 2011's
                            // `IFLG_STARTWITH=1` (acroom.h:2623, "start the player off with this
                            // item") exactly. Lands at EXACTLY 2011's declared `flags` offset
                            // (the struct's LAST field), zero drift.
  char _pad_align4[3];            // +0x41..0x44, compiler alignment padding (not a real field) --
                            // boxed in with zero slack by the confirmed total stride (0x44) and
                            // the confirmed `flags` field ending at +0x41.
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
  char _pad_align5[2];             // +0x853A..0x853C, compiler alignment padding (not a real
                            // field) -- `invinfo[100]` below is an array of a struct with 4-byte
                            // int members and needs 4-byte alignment; `numinvitems` (a short)
                            // ends 2 bytes short of that.
  InventoryItemInfo invinfo[100];  // +0x853C..0x9FCC (6800 bytes), high confidence: promoted from
                            // a totally unrecovered pad to CONFIRMED this round. A perfect
                            // zero-slack fit: `100 * sizeof(InventoryItemInfo) = 100*0x44 =
                            // 0x1A90` lands EXACTLY on the already-confirmed `numdialog`'s start
                            // once the 2-byte alignment pad above is included, and its own
                            // element-0 base (`dword_51B870`, the confirmed `invinfo[].pic`
                            // field minus its own +0x1C intra-element offset) lands EXACTLY 2
                            // bytes after the confirmed `numinvitems` ends. Matches
                            // OriGameSetupStruct's declared "short numinvitems;
                            // InventoryItemInfo invinfo[100]; int numdialog, numdlgmessage;"
                            // (acroom.h:2787-2789) adjacency exactly, with ZERO drift in element
                            // count (matching `mcurs[10]`/`iface[10]`'s own "not every array
                            // shrinks" pattern). See the `InventoryItemInfo` struct above for
                            // which of its own fields are independently confirmed (`pic`,
                            // `hotx`, `hoty`, `flags` -- all at their EXACT 2011-declared
                            // offsets, the strongest zero-drift match found in this project
                            // alongside `MouseCursor`).
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
  int reserved2[8];                // +0xA7F4..0xA814 (32 bytes), high confidence: promoted from
                            // unknown padding to CONFIRMED this round. No access-site evidence
                            // of its own (consistent with genuinely unused reserved space,
                            // matching its own name/intent), but boxed in with zero slack: it's
                            // the ONLY way `spriteflags` below (independently confirmed, see its
                            // own comment) lands at its own confirmed start address. Matches
                            // OriGameSetupStruct2's declared LAST field, `int reserved2[8]`
                            // (Common/acroom.h:2807), completing OriGameSetupStruct2's ENTIRE
                            // declared field list with zero remaining gaps.
  unsigned char spriteflags[6000]; // +0xA814..0xBF84 (6000 bytes), high confidence: `byte_51DB2C`.
                            // Confirmed via `prepare_characters_for_drawing` (already matched)
                            // THREE independent ways: (1) "xor eax,eax; mov al,byte_51DB2C[edx];
                            // and eax,2; test eax,eax; jz <skip>; ...var_C=final_col_dep..." --
                            // reads a per-sprite flags byte and checks bit 1 to gate hi-color-
                            // specific rendering, matching 2011's SPF_HICOLOR-style per-sprite
                            // flag semantics exactly, right alongside an already-confirmed
                            // sprite-width lookup (dword_4CD2E8[index*4]); (2) a direct sanity-
                            // clamp bounds check in the SAME function, "cmp index,0; jl <clamp>;
                            // cmp index,1770h; jl <ok>; clamp: index=0" -- `0x1770` (6000
                            // decimal) is a LITERAL constant, not an inferred fit, matching this
                            // array's capacity to the byte; (3) `+0xA814` plus 6000 bytes lands
                            // EXACTLY on GameSetupStructBase's own independently-confirmed total
                            // size (`0xBF84`, from the `fread`/`fwrite` `sizeof()` constant used
                            // throughout `load_ac2game_dta`/`SaveGameSlot`/`restore_game_data`)
                            // with zero slack -- `spriteflags` is the STRUCT'S OWN FINAL FIELD.
                            // DRIFT: `MAX_SPRITES=6000` in this build, not 2011's declared 30000
                            // (`acroom.h:2698`) -- a 5x capacity reduction, matching this
                            // project's established "smaller fixed capacity" pattern. Matches
                            // the further-derived `GameSetupStruct`'s declared `spriteflags`
                            // (`acroom.h:2893`) in identity, though this build has no separate
                            // base/derived split -- everything is flattened into one struct (see
                            // the MAJOR FINDING note below).
  //
  // STRUCT FULLY MAPPED: every byte of GameSetupStructBase, `+0x00` through `+0xBF84` (49028
  // bytes total), is now accounted for -- `spriteflags` above was the LAST remaining gap.
  // `numcursors`, `default_lipsync_frame`, `invhotdotsprite`, AND `default_resolution` are each
  // believed ABSENT from this build entirely (not merely unfound) -- see struct-layout-drift.md's
  // writeups on each (every runtime check for numcursors is hardcoded to 10; SetMouseCursor's
  // confirmed body has no invhotdotsprite-sprite branch; GetLipSyncFrame's two most distinctive
  // calls, strnicmp and strchr('/'), have zero matching candidates anywhere in the disassembly;
  // default_resolution's consuming code in `main` -- Engine/AC.CPP:27739-27782's whole
  // multi-branch native-resolution selector -- has NO counterpart at all in this build, which
  // instead hardcodes scrnwid/scrnhit=320x200 then branches purely on `usetup_screenres`, a
  // PLAYER/config setting (0/1/2 -> 320x200/640x400/960x600), not any `game.*` field -- confirmed
  // absent two more ways: `OriGameSetupStruct` never declares it at all, and 2011's own
  // `ConvertOldGameStruct` upgrade path (acroom.h:3017-3051) leaves it unset when upgrading an
  // old-format game -- unlike `numcursors`, which that same function explicitly hardcodes to 10
  // as a fallback. The whole "author declares a native game resolution, engine scales to fit"
  // feature is a later addition; this build's resolution is purely a player-side window-scale
  // choice).
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
  // STRUCT FULLY MAPPED: every byte from +0x00 to +0xBF84 (49028 bytes) is now confirmed --
  // `spriteflags[6000]` above was the LAST remaining gap, resolved via a direct `cmp index,1770h`
  // (6000) sanity-clamp bounds check in prepare_characters_for_drawing (already matched), landing
  // EXACTLY on this struct's own independently-confirmed total size with zero slack. No further
  // padding fields remain -- there is nothing left to recover in this struct's own layout.
  //
  // CRITICAL: unlike GUIButton/GUITextBox/etc. (dynamically allocated, no independently-known
  // total size to preserve), this struct's global instance already had a CONFIRMED exact size
  // (0xBF84, from the live IDB, predating this recovery) before any of the above fields were
  // named. Every field above was verified to keep the running total at exactly 0xBF84 before
  // being committed -- see struct-layout-drift.md for the full round-by-round history.
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

struct DialogTopic {
  // Total confirmed size 0x484 (1156 bytes), high confidence -- confirmed via the array
  // stride used by every access site below (`imul reg,484h`) AND via load_ac2game_dta's own
  // `malloc(numdialog*0x484+5)`/`fread(buffer,0x484,numdialog,stream)` allocation. UNLIKE every
  // other struct in this project, `Common/acroom.h` preserves NO older ancestor declaration for
  // `DialogTopic` to check this against -- 2011's own `DialogTopic` is ~4696 bytes
  // (`optionnames[MAXTOPICOPTIONS=30][150]` alone is 4500), over 4x bigger than this build's
  // confirmed total. This is a genuinely novel, from-scratch reconstruction, not a "confirm
  // against a known reference" exercise like every other struct tackled so far. The global
  // instance itself is a POINTER (`Engine/AC.CPP:567`'s `DialogTopic *dialog;`), dynamically
  // allocated based on the already-confirmed `GameSetupStructBase.numdialog` -- NOT embedded in
  // any other struct. `dword_4EDA48` in the current IDB corresponds to this pointer; consider
  // renaming it to `dialog` and typing it `DialogTopic *` manually in IDA (this project's
  // matches.json/apply_matches.py machinery only covers FUNCTION renames, not raw data
  // globals -- there's no automated path for this one).
  char optionnames[15][0x46];      // +0x000..0x41A (1050 bytes), HIGH confidence: promoted from
                            // an unresolved 1052-byte gap to CONFIRMED this round. Confirmed via
                            // `do_conversation` (already matched): the option-display loop does
                            // "imul eax,46h; add ecx,parmtr; push ecx; call GetTranslation" --
                            // passing `dialog[dlgnum] + chosenOption*0x46` directly to the
                            // newly-identified `GetTranslation` (`get_translation`), matching
                            // 2011's "get_translation(dtop->optionnames[disporder[ww]])"
                            // (AC.CPP:21874 etc.) in call shape and semantic role exactly. DRIFT:
                            // 15 options (not 2011's declared 30) of 0x46(70) chars each (not
                            // 2011's declared 150) -- both the element count AND the per-option
                            // text length are smaller, matching this project's established
                            // "reduced fixed capacity" pattern. `15*0x46=0x41A` leaves an exact
                            // 2-byte alignment pad before `optionflags` below, zero slack.
  char _pad_align2[2];             // +0x41A..0x41C, compiler alignment padding (not a real
                            // field) -- `optionflags` below is a 4-byte-int array and needs
                            // 4-byte alignment; `optionnames`'s 1050-byte extent lands 2 bytes
                            // short of that.
  int optionflags[15];             // +0x41C..0x458 (60 bytes), HIGH confidence: the element
                            // count (15, not 2011's declared `MAXTOPICOPTIONS=30`) is confirmed
                            // via an explicit `0x0F` (15) constant appearing TWICE, independently,
                            // in two different already-matched functions: `SaveGameSlot`'s
                            // "push 0Fh /* ElementCount */; push 4 /* ElementSize */; lea
                            // eax,[dialog+i*0x484+0x41C]; call fwrite" and `restore_game_data`'s
                            // matching `fread` counterpart with the identical `0Fh`/`4`/`+0x41C`
                            // triple -- a genuine save/restore round-trip of the topic's mutable
                            // per-option ON/OFF state (option TEXT/scripts are presumably static
                            // per-game-file and don't need saving, consistent with only
                            // `optionflags` being round-tripped here). Type/role also confirmed
                            // via `SetDialogOption` (already matched, script-exported): bitwise
                            // `AND ~1`/`OR 1` and `AND 2`/`OR 2` manipulation matches 2011's
                            // `DFLG_ON=1`/`DFLG_OFFPERM=2` (acroom.h) semantics exactly. DRIFT:
                            // `MAXTOPICOPTIONS=15` here, not 2011's 30 -- matching the
                            // established "smaller fixed capacity" pattern seen throughout this
                            // project (`ExecutingScript`'s `MAX_QUEUED_SCRIPTS=2`, `numcursors`
                            // hardcoded to 10, etc.).
  void *optionscripts;             // +0x458, high confidence: confirmed via load_ac2game_dta
                            // (already matched): "cmp [dialog+i*0x484+0x458],0; jz <skip>; ...
                            // Size=word[dialog+i*0x484+0x47C]+0xA; malloc(Size);
                            // [dialog+i*0x484+0x458]=result; fread(result,ElementSize=
                            // word[dialog+i*0x484+0x47C],1,stream)" -- an on-disk
                            // presence-flag-that-becomes-a-real-pointer idiom, the same pattern
                            // already confirmed for `compiled_script`/`dict` in
                            // GameSetupStructBase. Matches 2011's `unsigned char *optionscripts`
                            // (the compiled per-option dialog-script bytecode) in identity;
                            // position (right after `optionflags`) also matches 2011's declared
                            // adjacency exactly.
  short entrypoints[15];            // +0x45C..0x47A (30 bytes), MEDIUM confidence: NOT
                            // independently confirmed via its own access site -- included
                            // because it's boxed in with zero slack between the confirmed
                            // `optionscripts` and `startupentrypoint` fields, and 15 elements at
                            // 2 bytes each matches `MAXTOPICOPTIONS=15`, now confirmed THREE
                            // independent ways for this struct (`optionnames`, `optionflags`,
                            // and the `SaveGameSlot`/`restore_game_data` literal `0x0F` constant
                            // -- see their own comments), not just a coincidental gap size.
                            // Matches 2011's declared `short entrypoints[MAXTOPICOPTIONS]` in
                            // position and type.
  short startupentrypoint;         // +0x47A, high confidence: confirmed via `do_conversation`
                            // (already matched): "movsx edx,word[parmtr+0x47A]; push edx; push
                            // parmtr; call run_dialog_request" -- passed alongside the dialog
                            // topic pointer itself to `run_dialog_request` (already matched),
                            // matching 2011's semantic role ("initial dialog-script entry point
                            // to jump to") and position exactly.
  short codesize;                  // +0x47C, high confidence: confirmed via load_ac2game_dta
                            // (already matched) -- see `optionscripts` above: used directly as
                            // the `optionscripts` malloc/fread size, matching 2011's declared
                            // `short codesize` (the compiled bytecode's byte length) in both type
                            // and semantic role exactly.
  char _pad_align[2];              // +0x47E..0x480, compiler alignment padding (not a real
                            // field) -- `numoptions` below is a 4-byte int and needs 4-byte
                            // alignment; `codesize`'s 2-byte end lands short of that.
  int numoptions;                  // +0x480..0x484 (ending EXACTLY at the struct's own confirmed
                            // total size), high confidence: confirmed via `SetDialogOption`
                            // (already matched, script-exported): "cmp opt,[dlg+0x484i+0x480];
                            // setnle bl; ...quit(\"!SetDialogOption: Invalid option number\")" --
                            // an option-index range check, matching 2011's declared `int
                            // numoptions` in position and semantic role exactly. ARCHITECTURAL
                            // FINDING: `numoptions` ending EXACTLY at the struct's own confirmed
                            // total (0x484) means there is NO ROOM for 2011's `int topicFlags;`
                            // (2011's declared LAST field, right after `numoptions`) -- `topicFlags`
                            // is CONFIRMED ABSENT from this build, not merely unfound (the
                            // zero-slack arithmetic leaves no space for it at all).
};

struct MoveList {
  // Total confirmed size 0x200 (512 bytes), high confidence -- confirmed via `walk_character`'s
  // (already matched) own `shl eax,9` (2^9=512) stride scaling of the global array `mls`, AND
  // via SaveGameSlot's/restore_game_data's (already matched) literal `fwrite`/`fread` calls
  // using ElementSize=0x200 directly. The global instance itself, `mls` (an array, already
  // present with a PARTIAL `MoveList` type in the live IDB from before this project's own
  // tracking began -- only `pos`/`numstage` were previously named, via dot-notation access;
  // this struct's remaining fields were all unnamed raw-offset accesses until this round),
  // matches 2011's `MoveList mls[MAX_ROUTE_LISTS]`-equivalent global. EVERY field below matches
  // 2011's declared `Common/acroom.h:3082-3090` layout at its EXACT predicted offset with ZERO
  // drift -- as clean a match as `MouseCursor`/`InventoryItemInfo`.
  int pos[40];                     // +0x000..0x0A0 (160 bytes), high confidence: confirmed via
                            // `find_route` (already matched, `Engine/routefnd.cpp:766`): a
                            // direct `memcpy(&mls[movlst].pos, Src, numstage*4)` populates this
                            // array with the computed route waypoints. Also read by
                            // `walk_character` (already matched) to compare `pos[0]`/`pos[1]`
                            // (a "route has only one distinct waypoint" short-circuit check).
                            // Matches 2011's `int pos[MAXNEEDSTAGES=40]` (`acroom.h:3083`)
                            // exactly.
  int numstage;                    // +0x0A0, high confidence: confirmed via `find_route`
                            // (already matched): set to the route's stage count immediately
                            // before the `memcpy` into `pos[]` above, matching 2011's `int
                            // numstage` (`acroom.h:3084`) in both position and semantic role
                            // exactly.
  int xpermove[40];                 // +0x0A4..0x144 (160 bytes), MEDIUM confidence: NOT
                            // independently confirmed via its own access site this round --
                            // included because it's boxed in with zero slack between the
                            // confirmed `numstage` and `ypermove` fields, matching 2011's
                            // declared `fixed xpermove[MAXNEEDSTAGES]` (`fixed` is a 4-byte
                            // fixed-point int typedef, `acroom.h:3085`) in position and size.
  int ypermove[40];                 // +0x144..0x1E4 (160 bytes), MEDIUM confidence: same
                            // evidence status as `xpermove` immediately above -- boxed in with
                            // zero slack between it and the independently-confirmed `fromx`
                            // below. Matches 2011's declared `fixed ypermove[MAXNEEDSTAGES]`
                            // (`acroom.h:3085`) in position and size.
  int fromx;                       // +0x1E4, high confidence: confirmed via `find_route`
                            // (already matched): "mov dword ptr (mls+1E4h)[eax],ecx" sets it
                            // from a computed X coordinate as part of a "start of move" reset,
                            // matching 2011's declared `int fromx` (`acroom.h:3086`) exactly.
  int fromy;                       // +0x1E8, high confidence: confirmed the same way as `fromx`
                            // immediately above, in the same `find_route` block. Matches 2011's
                            // declared `int fromy` (`acroom.h:3086`) exactly.
  int onstage;                     // +0x1EC, high confidence: confirmed via `find_route`
                            // (already matched): set to 0 as part of the same "start of move"
                            // reset. Matches 2011's declared `int onstage` (`acroom.h:3087`)
                            // exactly.
  int onpart;                      // +0x1F0, high confidence: confirmed the same way as
                            // `onstage` immediately above, set to 0. Matches 2011's declared
                            // `int onpart` (`acroom.h:3087`) exactly.
  int lastx;                       // +0x1F4, high confidence: confirmed via `find_route`
                            // (already matched): set to `-1` (a "not yet drawn" sentinel) as
                            // part of the same reset. Matches 2011's declared `int lastx`
                            // (`acroom.h:3088`) exactly.
  int lasty;                       // +0x1F8, high confidence: confirmed the same way as `lastx`
                            // immediately above, set to `-1`. Matches 2011's declared `int
                            // lasty` (`acroom.h:3088`) exactly.
  char doneflag;                   // +0x1FC, high confidence: confirmed via `find_route`
                            // (already matched): set to `0` (byte-sized write, "mov byte ptr
                            // (mls+1FCh)[eax],0") as part of the same reset. Matches 2011's
                            // declared `char doneflag` (`acroom.h:3089`) exactly.
  char direct;                     // +0x1FD, MEDIUM confidence: NOT independently confirmed via
                            // its own access site this round -- boxed in with zero slack
                            // immediately after the confirmed `doneflag`. Matches 2011's
                            // declared `char direct` (`acroom.h:3090`, "MoveCharDirect was used
                            // or not") in position.
  char _pad_align[2];              // +0x1FE..0x200, compiler alignment padding (not a real
                            // field) -- boxed in with zero slack by the confirmed total stride
                            // (0x200) and the confirmed `direct` field ending at +0x1FE.
};

struct ViewFrame272 {
  // Per-frame struct, embedded inside each ViewStruct272 loop-block (see below). Total size
  // (0x1C, 28 bytes) is high confidence -- confirmed via multiple `imul reg,1Ch` frame-index
  // scaling sites in `update_stuff` (already matched). Two fields (`pic`, `speed`) now have
  // independent access-site evidence in this build; the remaining fields are still carried
  // over from 2011's declared `ViewFrame` (`Common/acroom.h:2268-2291`) as an UNVERIFIED
  // STRUCTURAL ASSUMPTION ONLY -- included so the 28-byte total has named fields instead of
  // opaque padding, not because each one has been independently checked. Do not cite
  // xoffs/yoffs/flags/sound/reserved_for_future as confirmed without new evidence. A dedicated
  // round searching for `flags` (the VFLG_FLIPSPRITE mirroring bit) came up empty -- every
  // already-matched frame-consuming function found this round (`prepare_characters_for_drawing`,
  // `AnimateObject`, `SetObjectView`/`SetObjectFrame`, `GetLocationType`'s mouse-cursor-animation
  // code) touches `pic` and/or `speed` only, never the flags/xoffs/yoffs/sound region. Shelved
  // for now, same status as `InterfaceElement`'s remaining fields -- revisit if a new caller
  // (a mirrored-sprite draw path, or a frame-linked sound-effect trigger) surfaces.
  int pic;                         // +0x00, high confidence: confirmed via `update_stuff`
                            // (already matched), TWICE, in a "loop has no frames of its own --
                            // fall back to the mirrored previous loop" check: "cmp dword ptr
                            // [loopbase+(frame-1)*1Ch], 0FFFFFFFFh" tests this exact field
                            // against the sentinel -1, matching 2011's convention of `pic==-1`
                            // meaning "this ViewFrame slot is unused" exactly (`acroom.h:2270`,
                            // "if (pic==-1) ..." pattern used throughout AC.CPP's own view code).
                            // ALSO confirmed via `SetObjectView`/`SetObjectFrame`/`AnimateObject`
                            // (all already matched), which each copy this field's low word into
                            // `RoomObject.num` when changing a room object's displayed frame.
  short xoffs;                     // +0x04, UNCONFIRMED -- see struct-level note above. Matches
                            // 2011's declared field in position only.
  short yoffs;                     // +0x06, UNCONFIRMED -- see struct-level note above.
  short speed;                     // +0x08, high confidence (UPGRADED from UNCONFIRMED):
                            // confirmed via `AnimateObject` (already matched, script-exported):
                            // "obj.wait = spdd + views[view].loops[loopn].frames[0].speed;" --
                            // reads this exact field (loop-block-relative +0x08, i.e.
                            // frame[0]'s own +0x08) and adds it to the caller-supplied speed
                            // parameter, matching 2011's `AnimateObject` (`AC.CPP:14872` area)
                            // in both call shape and semantic role ("per-frame speed offset
                            // added to the base animation speed") exactly.
  char _pad_align[2];              // +0x0A..0x0C, compiler alignment padding (not a real field)
                            // -- `flags` below is a 4-byte int and needs 4-byte alignment.
  int flags;                       // +0x0C, UNCONFIRMED -- see struct-level note above.
  int sound;                       // +0x10, UNCONFIRMED -- see struct-level note above.
  int reserved_for_future[2];       // +0x14..0x1C, UNCONFIRMED -- see struct-level note above.
};

struct ViewStruct272 {
  // Per-view struct, global array `views` (`dword_52313C`, a genuine standalone global pointer,
  // NOT embedded in GameSetupStructBase -- mirrors the earlier `dialog`/`DialogTopic` discovery
  // pattern) dynamically malloc'd/fread at load time. Total confirmed size 0x8D4 (2260 bytes),
  // high confidence -- confirmed via `load_ac2game_dta` (already matched): "imul eax,
  // ElementCount(numviews),8D4h; add eax,14h; push eax /* Size */; call malloc; ...; push
  // ElementCount; push 8D4h /* ElementSize */; push eax /* Buffer */; call fread" allocates and
  // reads exactly `numviews * 0x8D4` bytes. This 2260-byte size is FAR smaller than 2011's full
  // `ViewStruct272` (16 loops x 20 frames of 28-byte ViewFrame each, plus headers -- roughly
  // 9060 bytes computed from `Common/acroom.h:2421-2448`), meaning this 2002 build's view format
  // is even more reduced than the already-old 2011 `ViewStruct272` ancestor it's named after.
  // DRIFT: capacity is 8 loops x 10 frames/loop here (80 total frame slots) vs. 2011's declared
  // 16 loops x 20 frames (320 slots) -- a 4x reduction, consistent with this project's repeated
  // "smaller fixed capacity" pattern (see `run_another[2]` vs. `MAX_QUEUED_SCRIPTS=4`,
  // `optionnames[15]` vs. `MAXTOPICOPTIONS=30`, `spriteflags[6000]` vs. `MAX_SPRITES=30000`).
  //
  // The overall layout is an "over-determined fit": THREE independent facts all reconcile with
  // zero slack at once -- (1) the confirmed total view stride (0x8D4), (2) the confirmed
  // per-loop block stride (0x118=280 bytes, found via THREE separate disassembly sites: two in
  // `update_stuff`, one earlier in the still-unmatched `sub_40C3E0`), and (3) the confirmed
  // per-frame stride (0x1C=28 bytes, via multiple `imul reg,1Ch` sites in `update_stuff`).
  // 0x14(header) + 8*0x118(loop blocks) = 0x8D4 exactly; 0x118/0x1C = 10 frames/loop exactly;
  // 8*10*0x1C + 0x14 = 0x8D4 exactly. No combination of a different loop/frame count closes all
  // three simultaneously.
  short numloops;                  // +0x00, MEDIUM confidence: NOT independently confirmed via
                            // its own access site -- boxed in with zero slack immediately before
                            // the confirmed `numframes[]` below, matching 2011's declared field
                            // order (`short numloops; short numframes[16];`, `acroom.h:2421-2422`)
                            // exactly in position and type.
  short numframes[8];               // +0x02..0x12 (16 bytes), high confidence: confirmed via
                            // `update_stuff` (already matched), TWICE independently -- (1) the
                            // main frame-advance check: "movsx ecx,word[viewbase+curloop*2+2];
                            // cmp curframe,ecx; jl <keep-animating>" reads `numframes[curloop]` to
                            // decide whether to advance to the next frame or wrap to the next
                            // loop; (2) the loop-mirroring fallback check (see `ViewFrame272.pic`
                            // above) reads `numframes[curloop-1]` via the SAME array at
                            // `viewbase+curloop*2` (no +2, i.e. one loop index back) to find the
                            // mirrored loop's own last valid frame slot. Matches 2011's declared
                            // `short numframes[MAXLOOPSPERVIEW=16]` (`acroom.h:2422`) in position
                            // and type; DRIFT: capacity 8 here, not 16.
  char _pad_align[2];              // +0x12..0x14, compiler alignment padding (not a real field)
                            // -- boxed in with zero slack by the confirmed `numframes[8]` end and
                            // the confirmed loop-block start immediately below.
  ViewFrame272 frames[8][10];       // +0x14..0x8D4 (2240 bytes), high confidence: see the
                            // struct-level "over-determined fit" note above for the loop (0x118)
                            // and frame (0x1C) strides. DRIFT vs. 2011: no separate `loopflags[]`
                            // array exists in this build -- the header ends immediately after
                            // `numframes[8]` (zero slack, confirmed arithmetically: 0x02 + 0x10 =
                            // 0x12, +2 pad = 0x14, matching the loop-block start exactly, leaving
                            // no room for 2011's declared `int loopflags[MAXLOOPSPERVIEW]`,
                            // `acroom.h:2423`) -- loop-level flags (e.g. "run next loop" /
                            // RunNextLoop) are CONFIRMED ABSENT from this build's per-view data,
                            // not merely unfound.
};

struct RoomObject {
  // The current room's on-screen objects (props, animated background items -- AGS "Object"s
  // placed in the room editor, distinct from Characters). Global array `dword_4E45C8` is a
  // CACHED pointer, not a fresh allocation -- confirmed via `load_new_room` (already matched):
  // "mov eax, roomstats; add eax, newnum*1390h; mov dword_523128,eax" (or, for an out-of-range
  // room number, a fixed fallback "mov dword_523128, offset dword_4EF3A0") sets `dword_523128` =
  // `&roomstats[newnum]` (matching 2011's `croom=&roomstats[newnum]`, `Engine/AC.CPP:4259`,
  // `roomstats` being the confirmed global `RoomStatus *roomstats`, `AC.CPP:493`); immediately
  // after, "mov eax,dword_523128; add eax,8; mov dword_4E45C8,eax" sets `dword_4E45C8` =
  // `dword_523128 + 8` = `&roomstats[newnum].obj[0]`, matching 2011's declared `RoomStatus`
  // field order EXACTLY (`int beenhere; int numobj; RoomObject obj[MAX_INIT_SPR];` --
  // `Common/acruntim.h:94-97` -- `obj[]` starts immediately at `+0x08`, right after the two
  // confirmed leading ints). `RoomStatus.beenhere`@+0x00 and `.numobj`@+0x04 are independently
  // confirmed in the same `load_new_room` block ("cmp dword ptr[ecx],0" / "cmp ecx,[eax+4]" used
  // as a loop bound) -- `RoomStatus` itself is NOT formalized as its own struct yet (its total
  // size / `obj[]` capacity aren't independently confirmed for this build -- 2011 declares
  // `MAX_INIT_SPR=40`, `acroom.h:59`, not verified here), just these two leading fields plus the
  // `obj[]` array's own start address and element layout (this struct).
  //
  // IMPORTANT CORRECTION: an earlier round investigating `ViewStruct272` misattributed this
  // array's fields to a "per-character runtime animation-state array" (the loop variable in the
  // disassembly, IDA-named `chat`/`chaa`, was wrongly assumed to mean "character"). Reading
  // `SetObjectView`/`SetObjectFrame`/`GetObjectAt` (all already matched, all genuine AGS
  // room-object script API functions taking an object number `obn`/`obj`) directly against this
  // SAME array conclusively proves it is `RoomObject`, unrelated to `CharacterInfo`. See
  // reversing/notes/struct-layout-drift.md for the correction writeup.
  //
  // Total size 0x20 (32 bytes), high confidence: confirmed via the consistent `shl reg,5`
  // (2^5=32) stride scaling used by every function below. EVERY field is now HIGH confidence
  // (as of a follow-up round pulling in `SetObjectTransparency` and `AnimateObject`, both already
  // matched, which upgraded `transparent`@+0x08 and `overall_speed`@+0x1B from their initial
  // MEDIUM/boxed-only status) -- +0x00 through +0x1D is fully accounted for with no contradiction,
  // and +0x1E..0x20 is a natural 2-byte trailing alignment pad. As complete a match as
  // `MouseCursor`/`InventoryItemInfo`/`MoveList`.
  int x;                            // +0x00, high confidence: confirmed via `GetObjectAt`
                            // (already matched, script-exported `GetObjectAt(int xx,int yy)`):
                            // read directly as the object's on-screen X position, matching
                            // 2011's declared FIRST field (`Common/acruntim.h:39`) exactly.
  int y;                            // +0x04, high confidence: confirmed the same way as `x`
                            // immediately above, in the same `GetObjectAt` block. Matches 2011's
                            // declared field (`acruntim.h:39`) exactly.
  int transparent;                   // +0x08, high confidence (UPGRADED from MEDIUM): confirmed
                            // via `SetObjectTransparency` (already matched, script-exported): "if
                            // (trans==0) obj.transparent=0; else if (trans==100) obj.transparent=
                            // 255; else obj.transparent=((100-trans)*25)/10;" -- an EXACT
                            // instruction-for-instruction match to 2011's `SetObjectTransparency`
                            // (`Engine/AC.CPP:14679-14688`), all three branches writing this same
                            // offset. Also has an earlier, lower-confidence access site
                            // (`load_new_room`, already matched: "mov eax,word_51FF5C[chaa*0Ah];
                            // ...; mov [dword_523128+chaa*20h+8],eax" -- a per-object initial
                            // value copied from the compiled room file's own object-data table
                            // during room load). Matches 2011's declared field (`acruntim.h:40`)
                            // in position and semantic role exactly.
  short num;                        // +0x0C, high confidence: confirmed via THREE independent
                            // sites -- `GetObjectAt` uses it directly as a sprite-slot index into
                            // `dword_4E787C[]` (a sprite dimension lookup table) to compute the
                            // object's on-screen bounding box; `SetObjectView` and
                            // `SetObjectFrame` (both already matched) each set it from the low
                            // 16 bits of the relevant `ViewFrame272.pic` field ("mov cx,[view+
                            // loop*118h+frame*1Ch+14h]; mov [obj+0Ch],cx") immediately after
                            // changing the object's view/loop/frame -- a "narrow read" truncating
                            // `pic`'s low word, the same pattern seen elsewhere in this project
                            // (e.g. `InventoryItemInfo`). Matches 2011's declared `short num; //
                            // sprite slot number` (`acruntim.h:46`) in position and semantic role
                            // exactly.
  short baseline;                    // +0x0E, high confidence: confirmed via `GetObjectAt`
                            // (already matched): "cmp var,1; jge <use-as-is>; else var=y" --
                            // matching 2011's `get_baseline()` ("if (baseline<1) return y; return
                            // baseline;", `acruntim.h:66-70`) EXACTLY, both in logic shape and
                            // semantic role. Matches 2011's declared field (`acruntim.h:47`) in
                            // position and type.
  short view;                       // +0x10, high confidence: confirmed via `SetObjectView` and
                            // `SetObjectFrame` (both already matched, script-exported): both set
                            // this field directly from their `viw`/`vii` parameter. Matches
                            // 2011's declared `short view,loop,frame;` (`acruntim.h:48`) in
                            // position, type, and semantic role exactly.
  short loop;                       // +0x12, high confidence: confirmed via `SetObjectView`
                            // (already matched): reads this field back and compares it against
                            // `ViewStruct272.numloops` ("movsx ecx,word[obj+12h]; ...cmp
                            // ecx,word[view]; jl <ok>; else reset to 0") -- this is ALSO the
                            // evidence that independently upgrades `ViewStruct272.numloops`
                            // (`+0x00`) from MEDIUM to HIGH confidence, since it's read here with
                            // NO added offset. `SetObjectFrame` also sets it directly from its
                            // `lop` parameter. Matches 2011's declared field (`acruntim.h:48`)
                            // exactly.
  short frame;                      // +0x14, high confidence: confirmed via `SetObjectView`
                            // (reset to 0 when changing view) and `SetObjectFrame` (set directly
                            // from its `fra` parameter). Matches 2011's declared field
                            // (`acruntim.h:48`) exactly.
  short wait;                       // +0x16, high confidence: confirmed via `update_stuff`
                            // (already matched) -- see the `ViewStruct272` struct's own comments
                            // above for the frame-advance/loop-mirroring logic that reads and
                            // increments/decrements this field every frame the object is
                            // animating. Matches 2011's declared `short wait,moving;`
                            // (`acruntim.h:49`) in position exactly.
  short moving;                     // +0x18, high confidence: confirmed via `update_stuff`
                            // (already matched): passed BY ADDRESS ("lea ecx,[obj+18h]; push ecx;
                            // call do_movelist_move") to `do_movelist_move`, matching 2011's own
                            // call shape EXACTLY -- "do_movelist_move(&objs[aa].moving,&objs[aa].
                            // x,&objs[aa].y)" (`Engine/AC.CPP:6438`, `do_movelist_move`
                            // signature at `AC.CPP:17327`) confirms both the field identity AND
                            // the global's own identity as AGS's `objs[]` array. (This build's
                            // call site only pushes the one `mlnum` argument -- a reduced
                            // signature/calling convention compared to 2011's three-pointer
                            // version, consistent with this project's broader "build predates
                            // later refinements" pattern; not further investigated this round.)
                            // Matches 2011's declared field (`acruntim.h:49`) in position exactly.
  char cycling;                     // +0x1A, high confidence: confirmed via `SetObjectView` and
                            // `SetObjectFrame` (both already matched): both clear this field to 0
                            // immediately after changing the object's view/frame, matching 2011's
                            // exact semantic role ("is it currently animating?", `acruntim.h:50`
                            // -- explicitly setting a specific static frame stops any in-progress
                            // animation). ALSO confirmed via `AnimateObject` (already matched,
                            // script-exported): "obj.cycling = rept+1" when starting an animation
                            // -- matching 2011's exact "repeat count + 1" encoding convention
                            // (0 = not animating, nonzero = animating with that repeat setting).
                            // Matches 2011's declared field in position exactly.
  char overall_speed;                // +0x1B, high confidence (UPGRADED from MEDIUM): confirmed
                            // via `AnimateObject` (already matched, script-exported): "obj.
                            // overall_speed = (char)spdd;" -- sets it directly from the caller's
                            // `spdd` parameter when starting an animation. Matches 2011's declared
                            // `char overall_speed;` (`acruntim.h:51`) in position and semantic
                            // role exactly.
  char on;                          // +0x1C, high confidence: confirmed via `GetObjectAt`
                            // (already matched): "cmp byte[obj+1Ch],1; jz <hit-test-continue>" --
                            // an object must have `on==1` to be clickable/hit-testable, matching
                            // 2011's declared `char on;` (`acruntim.h:52`, object visibility) in
                            // position and semantic role exactly.
  char flags;                       // +0x1D, high confidence: confirmed via `GetObjectAt`
                            // (already matched), immediately after `on`: "movsx edx,byte[obj+
                            // 1Dh]; and edx,1; test edx,edx; jz <hit-test-continue>" -- a bit-0
                            // check gating hit-testability, matching 2011's declared `char
                            // flags;` (`acruntim.h:53`) in position and semantic role exactly.
                            // TWO specific bit values are now confirmed, both matching 2011's
                            // declared `OBJF_*` constants (`Common/acroom.h:792-798`) exactly:
                            // bit 0 (`and edx,1`, the `GetObjectAt` check above) = `OBJF_
                            // NOINTERACT`(1); bit 1 (`and edx,2`, `prepare_characters_for_
                            // drawing`, already matched: gates which of two draw paths is used
                            // -- a walk-behind-aware sort vs. a simpler direct draw) = `OBJF_
                            // NOWALKBEHINDS`(2).
  char _pad_align[2];               // +0x1E..0x20, compiler alignment padding (not a real field)
                            // -- boxed in with zero slack by the confirmed total stride (0x20)
                            // and the confirmed `flags` field ending at +0x1E.
};

struct EventBlockCmd {
  // A project-assigned name, NOT derived from any 2011 source identifier -- unlike every other
  // struct in this file, no living OR dead-commented 2011 declaration corresponds to this one.
  // Confirmed via two still-unnamed functions: sub_40C75E (called from `run_event_block`,
  // already matched, at offset +0x548 -- CONFIRMED to fire for `EventBlock.respond[i]==4`,
  // "Run Animation", via a direct "cmp respond[i],4" guard immediately before the call -- see
  // the `GameAnimation` struct below for the full resource-table discovery this unlocked) loops
  // "for (i=[arg_4]; i<[arg_0+0xF0]; i++) sub_40C3E0(&arg_0[i*0x18])" -- confirming the record
  // stride is 0x18(24 bytes) and that the list's own element-count field sits at list-relative
  // +0xF0. sub_40C3E0 is a COMPLETE per-record dispatcher, its
  // `type`@+0x14 byte now fully enumerated across all 6 values it handles (0-5; anything else
  // hits a SECOND distinct error, "unknown animation encountered", proving the switch is
  // exhaustive): type 0 = explicit error ("!undefined animation command"); type 1 = SetObjectView
  // (target is a room object) or SetCharacterView/ReleaseCharacterView (target is a character --
  // UNIFIED into one type here via a `data2==0` sentinel meaning "release", where 2011 later
  // SPLIT this into two separate command types, `case 27`/`case 28`); type 2 = AnimateObject
  // (object) or an inline animate-character equivalent (character, packing a value into
  // CharacterInfo+0x3E, not otherwise investigated); type 3 = `move_object`/`walk_character` with
  // `ignwal=0` (respect walkable areas); type 4 = the same, with `ignwal=1` ("move direct",
  // ignoring walls -- 2011's later `MoveObjectDirect`-style distinction, here just a flag on the
  // SAME command type rather than a separate one); type 5 = set the target's `x`/`y` position
  // directly with no movement at all (writes straight into `RoomObject.x`/`.y` for an object, or
  // `CharacterInfo+0x14`/`+0x18` for a character -- both already-confirmed offsets, reconfirmed
  // here via a brand new caller). This resembles 2011's `NewInteractionCommand`/
  // `run_interaction_commandlist` (`Common/acroom.h:600`, `Engine/AC.CPP:21449`) closely in ROLE
  // (nearly the same action set: set/release view, animate, move with/without wall-avoidance, set
  // position) but NOT in layout -- 2011's version has a vtable (virtual `NewInteractionAction`
  // base) and 5 typed `data[]` slots, totaling ~76+ bytes, vs. this build's flat 24-byte POD
  // record with 4 generic reused data slots. Given `run_event_block` (this build's confirmed
  // EventBlock processor) is the caller, and 2011 states the ENTIRE EventBlock/interaction-
  // scripts system "was replaced" by the time of the 2011 build (see run_event_block's own
  // matches.json entry), this is most plausibly EventBlock's own native command-list format for
  // its more complex response types -- a genuine ancestor of NewInteractionCommand that predates
  // the vtable/NewInteractionAction refactor AND predates the later split of "set view"/"release
  // view" and "move"/"move direct" into separate command types, not a corrupted or misread
  // version of the 2011 struct.
  int data0;                        // +0x00, high confidence: the target X coordinate for type 3/
                            // 4 (move, passed as `tox` to `move_object`/`walk_character`) and for
                            // type 5 (direct position set, written straight to `.x`). Unused by
                            // types 1/2. Matches 2011's generic `data[0]`/`IPARAM1` slot concept
                            // -- a fixed 4-byte argument whose meaning is entirely `type`-defined.
  int data1;                        // +0x04, high confidence: the target Y coordinate for type 3/
                            // 4/5 (`toy`, or written straight to `.y`), but REUSED for type 1/2
                            // as a single-bit flag (`and ecx,1`, a "repeat"-style gate) -- the
                            // clearest evidence this is a generic reusable slot (like 2011's
                            // `data[1]`/`IPARAM2`) rather than a dedicated "flags" field as
                            // originally guessed.
  int data2;                        // +0x08, high confidence: the view number for type 1
                            // (`SetObjectView`/`SetCharacterView`'s `vii`; `==0` selects
                            // `ReleaseCharacterView` instead for a character target) and the loop
                            // number for type 2's `AnimateObject` (object path). Unused by types
                            // 3/4/5. Matches 2011's `data[2]`/`IPARAM2`-in-those-cases slot.
  int target;                       // +0x0C, high confidence: the entity selector, decoded once
                            // near the top of sub_40C3E0 and used by every type -- this build's
                            // established object/character convention (<10 = room object index,
                            // ==99 = player character, >=100 = character index+100, matching the
                            // same convention documented for `AnimateObject`'s obn>=99 branch to
                            // `animate_character`).
  int data3;                        // +0x10, high confidence: the speed parameter for type 2's
                            // `AnimateObject` (object path, `spdd`) and type 3/4's move speed
                            // (`spee`, passed to `move_object`/packed into `walk_character`'s
                            // call); also packed (shifted left 8 bits) into CharacterInfo+0x3E
                            // for type 2's character path. Unused by types 1/5.
  char type;                        // +0x14, high confidence: the command type byte, fully
                            // enumerated 0-5 (see struct-level comment above) -- any other value
                            // hits an explicit "unknown animation encountered" error, proving no
                            // further types exist in this build.
  char waitUntilDone;                // +0x15, high confidence: gates a blocking `do_main_cycle`
                            // call after types 2 (character path), 3, and 4 -- "wait until this
                            // animation/movement finishes before continuing" (matching 2011's
                            // "wait until finished" idiom seen in `run_interaction_commandlist`'s
                            // `case 14: Move Object`). Not read by types 0/1/5 (which don't take
                            // time to complete).
  char _pad_align[2];               // +0x16..0x18, compiler alignment padding (not a real field)
                            // -- boxed in with zero slack now that every other byte through
                            // +0x15 is accounted for across all 6 command types.
};

struct GameAnimation {
  // A project-assigned name for a genuine, previously entirely unknown game resource: this
  // build's "Animations" system -- a room-independent, globally-numbered table of reusable
  // EventBlockCmd command lists, triggerable from any EventBlock's `respond[i]==4` ("Run
  // Animation") via `EventBlock.data[i]` as a 0-9 index. This matches the OLD AGS Editor's
  // "Animations" resource pane (a distinct project-tree entry type in ancient AGS versions,
  // predating the modern Views-only approach) -- entirely absent from the 2011 reference source,
  // consistent with "the whole EventBlock/interaction-scripts system was replaced" (see
  // run_event_block's matches.json entry) extending to this resource type as well.
  //
  // Confirmed via `run_event_block` (already matched): its `respond[i]==4` branch bounds-checks
  // `EventBlock.data[i]` against `0Ah`(10) -- erroring "!run_animate: undefined animation was
  // r[un]" if out of range -- giving MAX_ANIMATIONS=10 for this build. It then checks
  // `dword_52033C[data[i]*0xF4]` for nonzero, erroring "!Run_animate: empty animation was run"
  // otherwise, before finally calling sub_40C75E(&unk_52024C[data[i]*0xF4], 0) -- `unk_52024C`
  // being the actual table of this struct's instances, `sub_40C75E` being the already-
  // characterized EventBlockCmd list iterator (see its own entry).
  //
  // RESOLVED (a follow-up round, after this struct was first drafted): `dword_52033C` is NOT a
  // separate parallel table at all -- its address (0x52033C) is EXACTLY `unk_52024C`'s address
  // (0x52024C) plus `0xF0`, i.e. `&unk_52024C[0].numCommands`. IDA simply assigned it a distinct
  // symbol because the compiler generated that one access as a literal computed address rather
  // than as `unk_52024C+member_offset`, so IDA's data-xref analysis didn't recognize the overlap.
  // The "empty animation" check is therefore just `GameAnimation[data[i]].numCommands != 0` --
  // the exact same field `sub_40C75E`'s own loop bound reads, checked once early as an
  // error-message-friendly short-circuit before the (otherwise silently-no-op) iteration. This
  // ALSO gives `numCommands`@+0xF0 below a second, fully independent confirmation.
  //
  // Total size 0xF4(244 bytes) is high confidence: independently confirmed by TWO things landing
  // on it simultaneously -- the `dword_52033C`/`unk_52024C` address delta itself (`0xF0`, i.e.
  // exactly `numCommands`'s own offset), AND sub_40C75E's own confirmed "list[+0xF0] =
  // numCommands" access applied to `unk_52024C[slot]`, meaning `command[10]` (10*0x18=0xF0) plus
  // a trailing `numCommands` int lands EXACTLY on the externally-confirmed 0xF4 stride with zero
  // slack.
  EventBlockCmd command[10];         // +0x000..0xF0 (240 bytes), high confidence: see struct-level
                            // comment -- MAX_ANIMATIONS=10 slots confirmed via run_event_block's
                            // own literal `0Ah` bounds check, and this array's own per-element
                            // stride (0x18) already independently confirmed via EventBlockCmd.
  int numCommands;                   // +0xF0, high confidence: confirmed via sub_40C75E (already
                            // characterized): the loop bound for iterating `command[]`, matching
                            // the same role NewInteractionCommandList.numCommands plays in 2011's
                            // later, structurally unrelated replacement system. INDEPENDENTLY
                            // reconfirmed via `run_event_block`'s "empty animation" check reading
                            // this exact same field through the address it knows as
                            // `dword_52033C` -- see the struct-level RESOLVED note above.
};

struct RoomStatus {
  // The current room's own per-room save-state ("has this room been visited, what's changed
  // since"), matching 2011's declared `RoomStatus` (`Common/acruntim.h:94-97`) in NAME and
  // leading fields, though this build predates 2011's NewInteraction-based fields entirely (see
  // below). Global array `roomstats` (`RoomStatus *roomstats`, already confirmed via
  // `load_new_room`'s "dword_523128 = roomstats + newnum*1390h" pointer-chain evidence -- see
  // RoomObject's own struct comment) has `MAX_ROOMS`=300(0x12C) elements, confirmed via
  // `restore_game_data` (already matched): "cmp roomIndex, 12Ch; jge <done>" bounds a
  // save-file-restore loop over the WHOLE array -- matching 2011's declared `MAX_ROOMS=300`
  // (`acroom.h:789`) with ZERO drift, unusual for this project (most capacities are reduced).
  // Total per-element size 0x1390(5008 bytes), high confidence: confirmed via BOTH
  // `SaveGameSlot` and `restore_game_data` (both already matched) using it as the literal
  // `ElementSize` for a single `fwrite`/`fread` of the ENTIRE `roomstats` array as one raw blob
  // (plus a SEPARATE, size-prefixed transfer for the `tsdata` pointer field specifically, since
  // heap-allocated pointers can't be blindly blob-copied across save files -- see below).
  //
  // STRUCT FULLY MAPPED: every byte from +0x00 through +0x1390 is now accounted for. The
  // region 2011 fills with `NewInteraction`-based `intrHotspot`/`intrObject`/`intrRegion`/
  // `intrRoom` (already proven entirely absent from this build, see `EventBlockCmd`/
  // `GameAnimation`) turns out to hold this build's OWN EventBlock-based per-room interaction
  // data instead -- `hscond[20]`/`objcond[10]`/`misccond` below, matching 2011's OWN dead-
  // commented-out declaration for exactly this (`Common/acruntim.h:105-107`) almost verbatim.
  // `region_enabled` and `interactionVariableValues` are CONFIRMED ABSENT (see their own notes
  // below). 8 fields confirmed total, plus 2 confirmed absent.
  int beenhere;                     // +0x00, high confidence: confirmed via `load_new_room`
                            // (already matched): "cmp dword ptr[croom],0" gates a "first time
                            // entering this room" initialization block. Matches 2011's declared
                            // FIRST field (`acruntim.h:95`) exactly.
  int numobj;                       // +0x04, high confidence: confirmed via `load_new_room` and
                            // `sub_4256E0` (the RoomObject-number validity helper, already
                            // documented): the per-room object count, used as both an
                            // initialization loop bound and a live bounds check. Matches 2011's
                            // declared field (`acruntim.h:96`) exactly.
  RoomObject obj[10];                // +0x08..0x148 (320 bytes), MEDIUM confidence: NOT
                            // independently confirmed field-by-field this round -- derived from
                            // clean, zero-slack arithmetic between two independently confirmed
                            // anchors (`obj[]`'s own start at +0x08, already established via
                            // RoomObject's own discovery, and `tsdatasize`'s confirmed position
                            // at +0x168 below): `(0x168-0x08)=0x160`(352) bytes total for
                            // `obj[]`+`flagstates[]` combined; `352 - 15*2(flagstates, see
                            // below) - 2(align pad) = 320 = 10*0x20`(RoomObject's own confirmed
                            // stride) exactly -- the only clean integer solution once
                            // `flagstates[15]` is anchored to 2011's exact declared capacity.
                            // DRIFT: 10 objects/room here vs. 2011's declared
                            // `MAX_INIT_SPR=40` (`acroom.h:59`) -- a 4x reduction, consistent
                            // with this project's "smaller fixed capacity" pattern.
  short flagstates[15];              // +0x148..0x166 (30 bytes), MEDIUM confidence: same
                            // arithmetic-fit status as `obj[]` immediately above -- anchored to
                            // 2011's declared `short flagstates[MAX_FLAGS]` (`acruntim.h:98`,
                            // `MAX_FLAGS=15`, `acroom.h:801`) with ZERO drift, which is what
                            // makes the `obj[]`/`tsdatasize` arithmetic close cleanly to a round
                            // object count (10) rather than an arbitrary remainder.
  char _pad_align[2];                // +0x166..0x168, compiler alignment padding (not a real
                            // field) -- boxed in with zero slack by `flagstates[15]`'s odd
                            // (30-byte) size and the confirmed `tsdatasize` position immediately
                            // after.
  int tsdatasize;                    // +0x168, high confidence: confirmed via BOTH `SaveGameSlot`
                            // and `restore_game_data` (already matched): used as the
                            // `ElementSize`/`Size` for a size-prefixed `fwrite`/`malloc`+`fread`
                            // of the room's script data (`tsdata` below), gated by a `>0` check
                            // in both directions. Matches 2011's declared field
                            // (`acruntim.h:99`) in position and semantic role exactly.
  char *tsdata;                      // +0x16C, high confidence: confirmed via BOTH `SaveGameSlot`
                            // (the `fwrite` `Buffer` argument, sized by `tsdatasize`) and
                            // `restore_game_data` (freed if already allocated, then
                            // `malloc(tsdatasize+5)` and `fread` back in) -- matching 2011's
                            // declared `char* tsdata` (`acruntim.h:100`) in position and
                            // semantic role exactly, including the "free the old buffer before
                            // replacing it" pattern on restore.
  EventBlock hscond[20];              // +0x170..0xD00 (3760 bytes), high confidence: confirmed
                            // via `RunHotspotInteraction` (already matched): "imul ecx,hotspothere,
                            // 94h; mov edx,dword_523128(croom); lea eax,[edx+ecx+170h]" -- a
                            // per-hotspot EventBlock array starting IMMEDIATELY after `tsdata`,
                            // matching 2011's OWN dead-commented-out declaration exactly
                            // (`Common/acruntim.h:105`, "/* EventBlock hscond[MAX_HOTSPOTS]; ...
                            // */" -- 2011 replaced this with the NewInteraction-based
                            // `intrHotspot` array, but left the OLD EventBlock-based declaration
                            // as a comment, which is exactly what this build still has live).
                            // Capacity 20 matches the ALREADY-confirmed `hotspot_enabled[20]`
                            // capacity exactly (independent confirmation via a completely
                            // different access site), and the stride (`0x94`, EventBlock's own
                            // confirmed size) times 20 lands EXACTLY on the independently-
                            // confirmed `objcond` start below with zero slack.
  EventBlock objcond[10];             // +0xD00..0x12C8 (1480 bytes), high confidence: confirmed
                            // via `RunObjectInteraction` (already matched): "imul ecx,aa,94h; mov
                            // edx,dword_523128; lea eax,[edx+ecx+0D00h]" -- a per-object EventBlock
                            // array, matching 2011's dead-commented `EventBlock objcond[
                            // MAX_INIT_SPR]` (`acruntim.h:106`) in the same way as `hscond` above.
                            // Capacity 10 matches the ALREADY-confirmed `RoomObject obj[10]`
                            // capacity exactly (independent confirmation via a completely
                            // different access site: `obj[]`'s own count came from `SaveGameSlot`/
                            // `restore_game_data` arithmetic, `objcond`'s from a live script-API
                            // caller). `hscond[20] + objcond[10]` (`0x1160 + 0x5C8 = 0x1728` from
                            // `+0x170`) lands EXACTLY on `+0x12C8`, zero slack.
  EventBlock misccond;                // +0x12C8..0x135C (148 bytes), high confidence: confirmed
                            // via `sub_40C335` (already matched, called only from `new_room`):
                            // "mov ecx,dword_523128; add ecx,12C8h; ...; call run_event_block"
                            // with `String1="room"` -- the room-level EventBlock (2011's dead-
                            // commented `EventBlock misccond;`, acruntim.h:107, handling
                            // "Player Enters Screen"/"Player Leaves Screen" room events). Ends
                            // EXACTLY at the already-confirmed `hotspot_enabled` start (`+0x135C`)
                            // with zero slack -- the THIRD independent arithmetic convergence in
                            // this one small resolved region (hscond capacity, objcond capacity,
                            // and now misccond's own end position), closing the entire previously-
                            // opaque `+0x170..+0x135C` gap with real, caller-confirmed fields
                            // instead of an unexplored blob. All 9 `run_event_block` call sites in
                            // the binary are now accounted for: 2x `RunCharacterInteraction`
                            // (character-level, a SEPARATE global array, not part of RoomStatus),
                            // 2x `RunHotspotInteraction` (`hscond`), 2x `RunObjectInteraction`
                            // (`objcond`), 1x `sub_40C335`/`new_room` (`misccond`), 1x
                            // `run_event_block_inv` (inventory-level, also a separate array), and
                            // 1x `process_event`'s generic dispatch (routes to a pointer resolved
                            // elsewhere, not itself a new lead).
  char hotspot_enabled[20];           // +0x135C..0x1370 (20 bytes), high confidence: confirmed
                            // via FOUR independent already-matched sites all agreeing on this
                            // exact offset: `DisableHotspot`/`EnableHotspot` (both script-
                            // exported, "cmp hsnum,1; ...; cmp hsnum,14h(20); ..." bounds-checks
                            // hsnum to the range 1 through 19 inclusive before writing 0/1 to
                            // `croom[+0x135C+hsnum]`), `get_hotspot_at` (already matched: reads
                            // `croom[+0x135C+hsidx]`,
                            // "if zero, treat as no hotspot here" -- gating hotspot hit-testing
                            // on this exact flag), and `load_new_room` (already matched: a
                            // room-load init loop, "for(cc=0;cc<14h(20);cc++)
                            // croom[+0x135C+cc]=1", resetting every hotspot to enabled on room
                            // entry). DRIFT: capacity 20 here matches 2011's documented ORIGINAL
                            // `MAX_HOTSPOTS` value before it was later increased (`Common/
                            // acroom.h:65` comment: "v2.62 increased from 20 to 30; v2.8 to
                            // 50") -- this build genuinely predates both of those increases,
                            // a rare case of a confirmed field matching 2011's OLDEST
                            // documented capacity rather than a smaller ad-hoc reduction.
                            // Matches 2011's declared field (`acruntim.h:108`) in semantic role
                            // exactly, just not immediately adjacent to `tsdata` the way 2011
                            // declares it (2011 has several `NewInteraction`-based fields
                            // between them that this build doesn't have).
  short walkbehind_base[15];           // +0x1370..0x138E (30 bytes), high confidence: confirmed
                            // via `SetWalkBehindBase` (already matched, script-exported --
                            // currently mislabeled `SetalkBehindBase` in the IDB, a pre-existing
                            // typo, not introduced by this project): "cmp wa,1; ...; cmp wa,0Fh
                            // (15); ..." bounds-checks its `wa` argument to [1,14] before "mov
                            // [croom+wa*2+1370h], bl" (a 2-byte/short write, matching 2011's
                            // declared `short` element type exactly). Sits IMMEDIATELY after the
                            // confirmed `hotspot_enabled[20]` with ZERO gap -- this is also the
                            // decisive evidence that 2011's declared `region_enabled[MAX_REGIONS]`
                            // (`Common/acruntim.h`, between `hotspot_enabled` and
                            // `walkbehind_base`) is CONFIRMED ABSENT from this build, not merely
                            // unfound: there is no room for it at all between the two confirmed
                            // neighbors. Consistent with the independently-noted absence of
                            // `DisableRegion`/`EnableRegion`'s names AND error strings anywhere
                            // in this binary (unlike `DisableHotspot`/`EnableHotspot`, both
                            // present) -- this build most likely predates per-region
                            // enable/disable as a concept entirely, even though per-region
                            // tinting/light-level data already exists elsewhere (`SetAreaLightLevel`,
                            // already matched). DRIFT: capacity 15 (valid indices 1-14, plus an
                            // implicit reserved index 0, matching the same "index 0 = none"
                            // convention as `hotspot_enabled`) vs. 2011's declared `MAX_OBJ=16`
                            // (`Common/acroom.h:60`) -- a one-less reduction, unconfirmed whether
                            // this reflects a genuinely older `MAX_OBJ` value the way
                            // `hotspot_enabled`'s capacity did (no equivalent version-history
                            // comment exists for `MAX_OBJ` in the reference source).
  char _pad_align6[2];                // +0x138E..0x1390, compiler alignment padding (not a real
                            // field) -- boxed in with zero slack by the confirmed total stride
                            // (0x1390) and `walkbehind_base`'s own confirmed end at +0x138E. 2011's
                            // trailing `int interactionVariableValues[MAX_GLOBAL_VARIABLES]`
                            // (100 ints = 400 bytes) is CONFIRMED ABSENT here too -- there is no
                            // room for it at all, the struct simply ends 2 bytes after
                            // `walkbehind_base`.
};

struct WordsDictionary {
  // This build's flattened equivalent of 2011's `WordsDictionary` (`Common/acroom.h:341-344`),
  // which is a small header holding TWO separately dynamic-allocated arrays (`char **word`,
  // `short *wordnum`). This build instead uses ONE fixed-capacity heap blob, matching 2011's
  // field NAMES and semantic roles but not its indirection. Confirmed via `read_dictionary`
  // (already matched, `Common/acroom.h:1552`): "num_words = getw(Stream); for (i=0; i<num_words;
  // i++) { read_string_decrypt(Stream, &word[i]); fread(&wordnum[i], 2, 1, Stream); }" -- an
  // exact shape match to 2011's own `read_dictionary`, just operating on this build's flat
  // layout. Allocated in `load_ac2game_dta` (already matched) via a fixed `malloc(0xBB84)`
  // (48004 bytes) guarded by "if (dict-pointer-field != 0)" -- the same "pointer doubles as an
  // on-disk presence flag" idiom used elsewhere in this build (e.g. `compiled_script`). Sits
  // immediately after `GameSetupStructBase.numgui` with zero gap, matching `OriGameSetupStruct2`'s
  // declared adjacency `"int numgui; WordsDictionary *dict;"` (`acroom.h:2805-2806`) exactly --
  // see `GameSetupStructBase.dict`'s own confirmed offset.
  //
  // Capacity (1500 words) is high confidence, confirmed via ZERO-REMAINDER arithmetic from BOTH
  // ends simultaneously: `(wordnum_start - word_start) / word_stride` = `(0xAFCC-4)/30` = 1500
  // exactly, AND `(total_malloc_size - wordnum_start) / wordnum_stride` = `(0xBB84-0xAFCC)/2` =
  // 1500 exactly -- two independent divisions landing on the identical integer with no slack,
  // essentially ruling out coincidence.
  int num_words;                    // +0x0000, high confidence: confirmed via `read_dictionary`
                            // -- read directly via `getw()` as the loop bound for both `word[]`
                            // and `wordnum[]` below. Matches 2011's declared FIRST field
                            // (`acroom.h:342`) in position and role exactly.
  char word[1500][30];               // +0x0004..0xAFCC (44988 bytes), high confidence: confirmed
                            // via `read_dictionary` -- each entry read via `read_string_decrypt`
                            // (already matched) into a `word_stride=0x1E`(30)-byte slot, matching
                            // 2011's declared `MAX_PARSER_WORD_LENGTH=30` (`acroom.h:337`) with
                            // ZERO drift on the per-word capacity, even though the outer 1500-word
                            // TABLE capacity itself is a build-specific fixed bound with no 2011
                            // counterpart (2011's `word[]` is dynamically sized to `num_words`
                            // exactly, having no fixed ceiling at all).
  short wordnum[1500];                // +0xAFCC..0xBB84 (3000 bytes), high confidence: confirmed
                            // via `read_dictionary` -- each entry `fread` directly into a flat
                            // 2-byte slot. Matches 2011's declared `short *wordnum` (`acroom.h:
                            // 344`) in role (a per-word numeric ID for the text parser) with the
                            // same "no 2011 fixed ceiling" caveat as `word[]` above.
};

struct GameState {
  // FRESH SURVEY, ROUND 1 -- IN PROGRESS, deliberately partial. This is this build's version of
  // 2011's `play` global (`struct GameState play;`, Common/acruntim.h:465-618), the single
  // largest runtime-state struct in the whole engine (150+ fields in 2011, accumulated over
  // 9 years). Unlike most structs in this project, `play` is a plain global (not malloc'd), so
  // there is no allocation-size site to anchor a total struct size from -- boundaries here come
  // purely from .data layout adjacency and behavioral confirmation.
  //
  // MOST of the fields below (score through inv_numorder) were ALREADY named directly in the
  // live IDB from prior manual work (the same "already recovered, just needs formalizing"
  // precedent as SpriteCache) -- confirmed here to be a genuine tightly-packed run with zero
  // gaps in the raw .data listing, strongly reinforced by `play_globalvars`'s pre-existing
  // `32h dup(?)` = 50-dword declaration landing EXACTLY on 2011's `MAXGLOBALVARS=50`
  // (`acruntim.h:21`) with zero drift.
  //
  // STRUCTURAL QUESTION RESOLVED (fresh survey, later round): earlier rounds found dword_4EEB50
  // (in_cutscene) etc. computing to +0x138 from `play`'s base, but a global then labeled `ifnum`
  // appeared to sit at the intervening +0x110 position -- seemingly proving these couldn't be
  // the same contiguous object. That reasoning is now OVERTURNED: `SaveGameSlot` (already
  // matched) writes `play` directly to the save file with a LITERAL size constant --
  // "fwrite(&play, 0x964, 1, Stream)" (2404 bytes) -- landing with ZERO byte slack exactly where
  // an unrelated global, `String1`, begins. This proves GameState really is one contiguous
  // 2404-byte object end to end. `ifnum` turned out to be a MISLABELED field, not a genuine
  // separate global -- see `speech_textwindow_gui` below. See reversing/notes/
  // struct-layout-drift.md for the full correction writeup.
  int score;                  // +0x00, high confidence: confirmed via `replace_macro_tokens`
                            // (new match this round, sub_41024B, AC.CPP:7104) -- the GUI label
                            // "@SCORE@"/"@SCORETEXT@" macro-substitution routine reads the `play`
                            // global (this field) at exactly the two call sites matching source's
                            // two separate play.score reads.
  int usedmode;                // +0x04, medium-high confidence: pre-existing IDA name
                            // (`play_usedmode`), positionally exact; `ProcessClick` (already
                            // matched) writes it once via "play.usedmode=mood;" (AC.CPP:16680),
                            // matching the single write XREF'd here.
  int disabled_user_interface;  // +0x08, medium-high confidence: pre-existing IDA name, XREF'd
                            // from an as-yet-unmatched helper (sub_40C395) reading then writing
                            // it -- plausible role match, not yet individually confirmed.
  int gscript_timer;          // +0x0C, medium-high confidence: pre-existing IDA name, XREF'd from
                            // `load_new_room` (already matched, write) and `update_stuff`
                            // (already matched, read) -- plausible role match, not yet
                            // individually confirmed via a specific instruction.
  int debug_mode;              // +0x10, medium-high confidence: pre-existing IDA name, XREF'd
                            // from `debug_log` (already matched) and `check_controls` (already
                            // matched) -- plausible role match, not yet individually confirmed.
  int globalvars[50];         // +0x14..0xDC (200 bytes), high confidence: pre-existing IDA name
                            // AND pre-existing explicit `32h dup(?)` (50-entry) declaration,
                            // matching 2011's declared `int globalvars[MAXGLOBALVARS]`
                            // (`acruntim.h:471`, `MAXGLOBALVARS=50`) with ZERO drift -- a strong
                            // signal this is a real, deliberately-sized array, not coincidental
                            // padding. XREF'd from `setup_script_exports` (already matched) as an
                            // exported symbol, meaning this array is directly script-visible
                            // (matching 2011's own "obsolete" but still script-exposed role).
  int messagetime;             // +0xDC, medium-high confidence: pre-existing IDA name, XREF'd
                            // from `update_stuff` (already matched, read) -- plausible role
                            // match, not yet individually confirmed via a specific instruction.
  int usedinv;                 // +0xE0, medium confidence: pre-existing IDA name flagged with a
                            // trailing "?" by the prior (pre-this-project) manual naming pass,
                            // XREF'd from `check_controls` (already matched, read+write) --
                            // plausible but not individually confirmed; the prior session's own
                            // uncertainty flag is preserved here rather than silently upgraded.
  int inv_top;                 // +0xE4, HIGH confidence (UPGRADED from medium/"?"-flagged):
                            // confirmed via sub_40D80C (new match this round, algorithmic twin
                            // of 2011's offset_over_inv, AC.CPP:5394-5409) -- "mover +=
                            // play_inv_top" matches source's "mover += topIndex" exactly. The
                            // prior session's own uncertainty flag is now resolved.
  int inv_numdisp;             // +0xE8, HIGH confidence (UPGRADED from medium-high): confirmed
                            // via sub_40D80C -- "if (mover >= play_inv_numdisp) return -1"
                            // matches source's "if (mover >= itemsPerLine*numLines) return -1"
                            // exactly, i.e. inv_numdisp is a precomputed itemsPerLine*numLines
                            // total-capacity value.
  int inv_numorder;            // +0xEC, HIGH confidence (UPGRADED from medium-high): CONFIRMED
                            // NOT obsolete in this build, resolving the prior round's open
                            // question -- `update_invorder` (already matched) directly
                            // increments this field as its LIVE running counter
                            // ("play_inv_numorder=0; ... play_inv_numorder++;"), and
                            // sub_40D80C bounds-checks against it exactly like 2011's
                            // "invorder_count". 2011 keeps `obsolete_inv_numorder`
                            // (`acruntim.h:474`) only as a single backwards-compatibility mirror
                            // of the real per-character count; this build has no such
                            // generalization yet -- this IS the one true counter.
  int inv_numinline;          // +0xF0, high confidence (UPGRADED from tentative): confirmed via
                            // sub_40D80C as `itemsPerLine` -- "mover = mouseX/(item_wid*mult_x);
                            // if (mover < inv_numinline) { ... }" matches source's "mover =
                            // xoffs/itemWidth; if (mover >= itemsPerLine) return -1;" (inverted
                            // branch sense) exactly, and the field name itself ("number in a
                            // line") matches 2011's declared role precisely
                            // (`acruntim.h:474`-adjacent field, "inv_numinline").
  int _tentative_text_speed;  // +0xF4, TENTATIVE, positional inference only: the 12-byte gap
                            // between the newly-confirmed `inv_numinline`@+0xF0 and
                            // `inv_item_wid`@+0x100 is EXACTLY 3 ints, matching the count of
                            // fields 2011 declares in that same span (`text_speed`,
                            // `sierra_inv_color`, `talkanim_speed`) with zero slack -- a
                            // positional over-determined fit, but none of these three has an
                            // access-site confirmation yet.
  int _tentative_sierra_inv_color; // +0xF8, TENTATIVE, see `_tentative_text_speed` above.
  int _tentative_talkanim_speed;   // +0xFC, TENTATIVE, see `_tentative_text_speed` above.
  int inv_item_wid;            // +0x100, high confidence: confirmed via sub_40D80C as the
                            // column-width divisor -- "mover = mouseX/(inv_item_wid*mult_x)"
                            // matches source's "mover = xoffs/itemWidth" (`GUIInv::itemWidth`,
                            // set by `SetInvDimensions`, AC.CPP:24106) exactly. Matches 2011's
                            // exact declared field name and adjacent pairing with `inv_item_hit`.
  int inv_item_hit;            // +0x104, high confidence: confirmed via sub_40D80C as the
                            // row-height divisor, same match shape as `inv_item_wid` above.
  int _tentative_speech_text_shadow;   // +0x108, TENTATIVE, positional inference only: the
                            // 8-byte gap between `inv_item_hit`@+0x104 and the newly-confirmed
                            // `speech_textwindow_gui`@+0x110 (below) is exactly 2 ints, matching
                            // 2011's declared field count in that same span (`speech_text_shadow`,
                            // `swap_portrait_side`) with zero slack -- no access-site evidence yet.
  int _tentative_swap_portrait_side;   // +0x10C, TENTATIVE, see field above.
  int speech_textwindow_gui;  // +0x110, high confidence: this is what the pre-existing IDA
                            // global label `ifnum` (address 0x4EEB28) actually is -- NOT a
                            // genuinely separate variable as earlier rounds assumed (see this
                            // struct's own header comment for the correction). `main` (already
                            // matched) contains "ifnum = game_options[OPT_TWCUSTOM]; if
                            // (ifnum==0) ifnum=-1;", matching 2011's "play.speech_textwindow_gui
                            // = game.options[OPT_TWCUSTOM]; if (play.speech_textwindow_gui==0)
                            // play.speech_textwindow_gui=-1;" (AC.CPP:26389-26391) exactly. Its
                            // computed offset (0x4EEB28-0x4EEA18=0x110) also lines up exactly
                            // with 2011's declared position (3 fields after inv_item_hit) if
                            // this build has zero drift in that short span.
  char _pad_unknown1a[0x04];   // +0x114..0x118, genuinely unexplored (1 dword, dword_4EEB2C) --
                            // XREF'd from `update_stuff` (already matched) negating it into a
                            // walk-target-adjacent field; role not pinned down this round.
  int totalscore;               // +0x118, high confidence: `replace_macro_tokens` (already
                            // matched) reads this for BOTH its "totalscore" and "scoretext"
                            // macro branches, matching 2011's `#define MAXSCORE
                            // play.totalscore` (`acruntim.h:809`) exactly -- 2011's own source
                            // uses the macro rather than the field name directly at both call
                            // sites (`AC.CPP:7134`/`7136`), but the macro's definition makes
                            // the identification unambiguous.
  int _tentative_skip_display;  // +0x11C, MEDIUM confidence: `_display_main` (already matched)
                            // checks this against 0/2/3 in a message-box wait loop deciding
                            // whether to poll for a skipping keypress/mouseclick -- generally
                            // consistent with GameState.skip_display's role ("how the user can
                            // skip normal Display windows") and small-int-enum value space
                            // (2011's sibling field skip_speech uses the same style of enum via
                            // user_to_internal_skip_speech, AC.CPP:12790-12810), but this
                            // build's specific branch structure doesn't cleanly match a single
                            // identifiable 2011 function line for line -- not upgraded past
                            // medium confidence pending a more direct behavioral match.
  char _pad_unknown1c[0x04];   // +0x120..0x124 (1 dword, dword_4EEB38), genuinely unexplored --
                            // XREF'd from `update_stuff` (already matched) gating a per-
                            // character blink/talk-animation update block; role not pinned
                            // down this round.
  int roomscript_finished;      // +0x124, high confidence: `post_script_cleanup` (already
                            // matched)'s `runnext[0]=='$'` branch does
                            // "run_text_script_iparam(roominst,...); dword_4EEB3C=1;" matching
                            // 2011's "run_text_script_iparam(roominst,&runnext[1],...);
                            // play.roomscript_finished = 1;" (AC.CPP:3179-3181) exactly.
  int used_inv_on;              // +0x128, high confidence: `check_controls` (already matched)'s
                            // GOBJ_INVENTORY click branch does "iit=sub_40D80C(); if (iit>=0)
                            // dword_4EEB40=iit;" matching 2011's "int
                            // iit=offset_over_inv(...); if (iit>=0) { ...; play.used_inv_on =
                            // iit; }" (AC.CPP:5707-5710) exactly -- also a further independent
                            // confirmation that sub_40D80C is this build's offset_over_inv
                            // equivalent.
  char _pad_unknown1d[0x04];   // +0x12C..0x130 (1 dword, dword_4EEB44), genuinely unexplored --
                            // an earlier round's `skip_display` guess for this specific field
                            // is RETRACTED in favor of `_tentative_skip_display`@+0x11C above,
                            // which has closer-matching evidence (the same small-int value
                            // space AND a message-box-skip-specific role, vs. this field's
                            // weaker "set to 2 under some unrelated gate" evidence). XREF'd
                            // from an `_display_at`-adjacent function (sub_4141B8, already
                            // touched but not fully read this round).
  int max_dialogoption_width;   // +0x130, high confidence: `do_conversation` (already matched)
                            // computes "wii = dword_4EEB48 * current_screen_resolution_
                            // multiplier_x" inside its is_textwindow-equivalent branch,
                            // matching 2011's "areawid = multiply_up_coordinate(play.
                            // max_dialogoption_width);" (`AC.CPP:22119`) exactly in role and
                            // context (dialog-options text-window width computation).
  int no_hicolor_fadein;        // +0x134, medium-high confidence: an unmatched helper
                            // (sub_40A6D8, called from the already-matched `FadeIn`/
                            // `process_event`) gates a hi-color-depth-specific fast path on
                            // this flag ("if (color_depth>1) { if (dword_4EEB4C) {
                            // fast_path(); } else { full_fadein_path(); } }"), matching 2011's
                            // `no_hicolor_fadein`'s role (`AC.CPP:3495-3497`, "fade out but
                            // instant in for hi-color") closely but not as a clean line-for-
                            // line structural match, hence medium-high rather than high.
  int in_cutscene;              // +0x138, high confidence: check_skip_cutscene_keypress
                            // (already matched) -- "(play.in_cutscene>0) &&
                            // (play.in_cutscene!=3)" exact branch match.
  int fast_forward;             // +0x13C, high confidence: FadeOut (already matched)
                            // bails out immediately "if (dword_4EEB54!=0) return;",
                            // matching AGS's extremely common "if (play.fast_forward)
                            // return;" early-bailout idiom (41 occurrences in AC.CPP,
                            // e.g. write_screen()'s identical first-line gate,
                            // AC.CPP:2776-2777) -- also matches 2011's own declared
                            // adjacency to in_cutscene ("int in_cutscene; int
                            // fast_forward;", acruntim.h:496-497) with zero drift.
  int bg_frame;                 // +0x140, HIGH confidence (upgraded via a second
                            // independent confirmation, see mainloop below) -- zeroed in
                            // the EndSkippingUntilCharStops/unload_old_room-combined
                            // function (sub_40AAE3) matching source's paired
                            // "play.bg_frame=0; play.bg_frame_locked=0;"
                            // (AC.CPP:3624-3625), AND incremented/wrapped in mainloop
                            // (sub_42106D, already matched) matching source's
                            // "play.bg_frame++; if (play.bg_frame>=thisroom.num_bscenes)
                            // play.bg_frame=0;" (AC.CPP:25573-25575) exactly.
  int bg_anim_delay;            // +0x144, high confidence: mainloop (already matched)
                            // matches 2011's "if (play.bg_anim_delay>0)
                            // play.bg_anim_delay--; else if (play.bg_frame_locked) ;
                            // else { play.bg_anim_delay=play.anim_background_speed;
                            // play.bg_frame++; ...}" (AC.CPP:25569-25575) instruction
                            // for instruction. Bonus: identifies dword_52308C
                            // (standalone global) as GameState.anim_background_speed.
  short wait_counter;           // +0x148, high confidence: pre-existing IDA name
                            // (`play_wait_counter`), positionally exact (see SetMouseBounds
                            // evidence for the four fields right after it).
  short mboundx1;                // +0x14A, high confidence via SetMouseBounds (already
                            // matched) -- see its own evidence.
  short mboundx2;                // +0x14C, high confidence, see mboundx1.
  short mboundy1;                // +0x14E, high confidence, see mboundx1.
  short mboundy2;                // +0x150, high confidence, see mboundx1.
  int fade_effect;               // +0x154, high confidence: sub_40AAE3's 3-way screen-
                            // transition dispatch ("cmp dword_4EEB6C,1 / cmp
                            // play_scren_tint,0 / cmp dword_4EEB6C,0") matches 2011's
                            // current_fade_out_effect() (AC.CPP:3538-3567) exactly,
                            // using FADE_NORMAL=0/FADE_INSTANT=1 (acroom.h:2753-2754).
                            // Matches 2011's own declared adjacency to bg_frame_locked
                            // immediately below (acruntim.h:550-551) with zero drift.
  int bg_frame_locked;           // +0x158, high confidence: TRIPLY confirmed --
                            // zeroed matching source's "play.bg_frame_locked=0;" in
                            // sub_40AAE3, sits immediately after fade_effect matching
                            // 2011's exact declared order, AND checked in mainloop's
                            // bg_frame-advance gate matching source exactly.
  int globalscriptvars[300];    // +0x15C..0x60C (1200 bytes), high confidence via SetGlobalInt
                            // (already matched) -- see its own evidence. DRIFT: 300 here vs.
                            // 2011's declared MAXGSVALUES=500.
  char _pad_unexplored2[0x200]; // +0x60C..0x80C (512 bytes), genuinely unexplored -- KNOWN to
                            // contain at least one whole unrelated struct, not just uncharted
                            // GameState fields: `word_4EF0F4`/`word_4EF158`/`word_4EF1BC`
                            // (+0x6DC from play's base) turned out to be this build's
                            // `CharacterExtras.width`/`.height`/`.zoom` (three parallel
                            // `short[50]` arrays -- structure-of-arrays, unlike 2011's single
                            // `CharacterExtras charextra[50]` array-of-structs, Common/
                            // acruntim.h:441-455; see `prepare_characters_for_drawing`'s own
                            // matches.json entry for the full evidence chain). This PROVES
                            // SaveGameSlot's literal 0x964 fwrite constant (see this struct's
                            // header comment) sweeps in more than just the true GameState
                            // struct -- it also captures adjacent-but-distinct AC.CPP file-scope
                            // globals (here, a WHOLE SEPARATE real struct, not just scratch
                            // memory) that the linker happened to place contiguously after
                            // `play`. IMPORTANT CAUTION for the rest of this pad and the one
                            // below: falling inside the fwrite's span is NOT by itself
                            // sufficient evidence of GameState membership -- role-based
                            // confirmation is still required, as it was for every field already
                            // confirmed in this struct (in_cutscene, speech_textwindow_gui,
                            // totalscore, walkable_areas_on below, etc.).
                            //
                            // `play_scren_tint`/screen_tint is CONFIRMED ABSENT here (its own
                            // address computes to +0x10C6C, far outside GameState's bounds --
                            // it really is a standalone global). `play_invorder` (this build's
                            // `short[100]` inventory-order array -- capacity confirmed via a
                            // clean, zero-interruption 200-byte span immediately after it,
                            // matching MAX_INV=100 with zero drift -- see update_invorder's
                            // evidence) computes to +0x614 -- still unresolved whether it's a
                            // genuine GameState member or, like CharacterExtras, a
                            // coincidentally-adjacent separate global; not added as a typed
                            // struct member pending role-based confirmation or further mapping
                            // of the surrounding territory.
  char walkable_areas_on[16]; // +0x80C..0x81C, high confidence: `EndSkippingUntilCharStops`/
                            // `unload_old_room`-combined (sub_40AAE3, already matched) does
                            // "memset(&byte_4EF224, 1, 0x10);" matching 2011's
                            // "memset(&play.walkable_areas_on[0],1,MAX_WALK_AREAS+1);"
                            // (AC.CPP:3623) exactly -- MAX_WALK_AREAS=15 (acroom.h:250), so
                            // MAX_WALK_AREAS+1=16=0x10 with zero drift.
  short _tentative_screen_flipped; // +0x81C, TENTATIVE, positional inference only: the 2-byte
                            // gap between the newly-confirmed walkable_areas_on's end (+0x81C)
                            // and the already-confirmed offsets_locked (+0x81E, below) matches
                            // 2011's declared adjacency "char walkable_areas_on[...]; short
                            // screen_flipped; short offsets_locked;" (acruntim.h:556-558) with
                            // zero slack for exactly this one intervening field -- no direct
                            // access-site evidence of its own yet.
  short offsets_locked;    // +0x81E, high confidence: originally found via sub_40AAE3 zeroing
                            // it immediately after bg_frame_locked (matching source's exact
                            // adjacent assignment order), and its GameState membership -- left
                            // as an open question two rounds ago after the CharacterExtras
                            // correction -- is now REINFORCED by a second, independent,
                            // positional confirmation: it lands exactly 2 bytes after the newly
                            // and separately confirmed walkable_areas_on, matching 2011's own
                            // declared field order with zero slack.
  char _pad_unexplored3[0x18]; // +0x820..0x838 (24 bytes), genuinely unexplored -- contains at
                            // least dword_4EF240 (XREF'd from load_new_room) and dword_4EF248
                            // (XREF'd from check_controls, itself checked against 0/2/3 the
                            // same way the tentative skip_display candidate was -- not chased
                            // further this round, possibly a related or duplicate lead).
  int script_timers[21];       // +0x838..0x88C (84 bytes), high confidence: `update_stuff`
                            // (already matched)'s own OPENING lines loop "for(chat=0;chat<15h;
                            // chat++) if(dword_4EF250[chat]>1) dword_4EF250[chat]--;" matching
                            // 2011's "for (aa=0;aa<MAX_TIMERS;aa++) { if
                            // (play.script_timers[aa]>1) play.script_timers[aa]--; }"
                            // (AC.CPP:6431-6433) instruction for instruction, including the
                            // loop bound (0x15=21=MAX_TIMERS, acruntim.h:431).
  int sound_volume;             // +0x88C, high confidence: an unmatched helper (sub_4089CC,
                            // called from PlayAmbientSound/SetSoundVolume, both already
                            // matched) computes "vol*dword_4EF2A4/255" matching 2011's
                            // "ambientvol = (sourceVolume*play.sound_volume)/255;"
                            // (AC.CPP:1567) exactly. Sits with ZERO gap immediately after
                            // script_timers, matching 2011's exact declared adjacency
                            // "script_timers[MAX_TIMERS]; sound_volume,speech_volume;" with
                            // zero drift.
  int speech_volume;            // +0x890, high confidence: an unmatched helper (sub_4141B8,
                            // called from _display_at, already matched) passes dword_4EF2A8 as
                            // a volume argument to WAV-then-MP3 speech-file-loading helpers in
                            // sequence, matching 2011's "speechmp3=my_load_wave(finame,
                            // play.speech_volume,0); ... speechmp3=my_load_mp3(finame,
                            // play.speech_volume);" (AC.CPP:13387-13396) exactly. Sits with
                            // ZERO gap immediately after sound_volume, matching 2011's exact
                            // declared pairing with zero drift.
  char _pad_unexplored4[0xA4]; // +0x894..0x938 (164 bytes), genuinely unexplored -- 2011
                            // declares a large run of fields here (normal_font/speech_font
                            // through parsed_words[]/bad_parsed_word[100]), almost certainly
                            // not all present given this project's repeated massive-drift
                            // pattern; not mapped this round.
  int raw_color;                // +0x938, high confidence: `RawSetColor` (already matched,
                            // mechanical) does "dword_4EF350 = get_col8_lookup(this);"
                            // matching 2011's "play.raw_color = get_col8_lookup(clr);"
                            // (AC.CPP:14434) exactly. Sits with ZERO gap immediately before
                            // filenumbers below, CONFIRMING 2011's declared
                            // raw_modified[MAX_BSCENE] is ABSENT here -- no room for it at all.
  short filenumbers[20];        // +0x93C..0x964 (40 bytes), high confidence: pre-existing IDA
                            // name (`play_filenumbers`), capacity confirmed via
                            // `ListBoxSaveGameList` (already matched)'s own sort/swap loop
                            // bound ("cmp [var],14h", 20 -- matching Engine/acdialog.h:870's
                            // MAXSAVEGAMES=20, not acruntim.h's own separate MAXSAVEGAMES=50
                            // definition). THIS IS GAMESTATE'S LAST FIELD: it ends EXACTLY at
                            // +0x964, the struct's own independently-proven total size
                            // (SaveGameSlot's literal fwrite ElementSize, see this struct's
                            // header comment) -- zero remaining bytes, closing the tail
                            // completely. A parallel array, word_4EF356[20], is swapped
                            // alongside filenumbers in the same sort loop but sits just OUTSIDE
                            // GameState's proven bounds -- plausibly a save-slot sort-key/
                            // timestamp array, not part of this struct, not independently
                            // identified.
};

// CharacterExtras (this build's version) -- found while mapping GameState's unexplored tail,
// initially misread as more render-time scratch before its actual role was confirmed (see
// prepare_characters_for_drawing's matches.json entry for the full correction). NOT declared as
// a single struct type here: unlike 2011's `CharacterExtras charextra[50]` (array-of-structs,
// Common/acruntim.h:441-455), this build implements it as THREE SEPARATE PARALLEL short[50]
// arrays (structure-of-arrays) -- a genuine memory-layout difference, matching the same
// "flattened/simplified 2002 predecessor" pattern seen elsewhere in this project (e.g.
// ExecutingScript, GameAnimation), just applied to array-of-structs vs. structure-of-arrays
// rather than field count. Address range: 0x4EF0F4..0x4EF220 (300 bytes total, 3*50*2).
//   short char_width[50];   // word_4EF0F4, high confidence: matches 2011's
//                           // "scale_sprite_size(sppic,zoom_level,&newwidth,&newheight);
//                           // charextra[aa].width=newwidth;" (AC.CPP:8392-8393) exactly.
//   short char_height[50];  // word_4EF158, high confidence: same source call/statement pair
//                           // as char_width above, "charextra[aa].height=newheight;"
//                           // (AC.CPP:8393-8394).
//   short char_zoom[50];    // word_4EF1BC, high confidence: matches 2011's "zoom_level =
//                           // charextra[aa].zoom; if (zoom_level==0) zoom_level=100;"
//                           // (AC.CPP:8309-8312) read side, and the field's own write-back
//                           // after computation matches "charextra[aa].zoom=zoom_level;".
// Capacity 50 (vs. 2011's declared array size, not independently checked against a version-
// history comment the way some other capacity-drift findings in this project have been) --
// confirmed via a clean, zero-interruption 100-byte (50-short) span between each of the three
// arrays with no other labels breaking it. Base sprite dimensions feeding the width/height
// scale computation, dword_4CD2E8[]/dword_4E787C[], are plausibly `spritewidth[]`/
// `spriteheight[]` (both well-known AGS globals, Engine/acplatfm.cpp:438 etc.) -- medium-high
// confidence, not independently confirmed this round.
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
              "InventoryItemInfo, GameSetupStructBase, ExecutingScript, DialogTopic, "
              "MoveList, ViewFrame272, ViewStruct272, RoomObject, EventBlockCmd, "
              "GameAnimation, RoomStatus, WordsDictionary, GameState).")
    else:
        print(f"parse_decls reported {err_count} error(s) -- check the declarations above.")


if __name__ == "__main__":
    main()
