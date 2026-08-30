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
  char vtext[4];             // +0x00, high confidence (UPGRADED from MEDIUM): confirmed via a
                           // newly-found `GUIMain::init()`-equivalent (see the struct-level round
                           // note below this field list for the complete writeup and its important
                           // caveat -- it has NO formal IDA function boundary yet): its very first
                           // instruction is "mov byte ptr [this], 0" -- a single-BYTE write,
                           // matching source's "vtext[0]=0;" (`acgui.cpp:987`) exactly in both
                           // value and type (a char-array element write, not a 4-byte int write --
                           // independently confirming this is genuinely a `char[]` start, not some
                           // other field). 2011's own comment calls this "for compatibility" --
                           // likely already vestigial even in 2011, plausibly more so here.
  char name[16];              // +0x04, MEDIUM confidence: positional/arithmetic fit only, matching
                           // 2011's declared `char name[16]; // the name of the GUI`.
  char clickEventHandler[20]; // +0x14, MEDIUM confidence (byte offset still positional/arithmetic
                           // fit only), but the ASSOCIATED BEHAVIOR is now confirmed absent: 2011's
                           // `process_interface_click` (`Engine/AC.CPP:5355-5391`) reads this field
                           // only in its `btn<0` ("click on GUI background") branch --
                           // `run_text_script_2iparam(gameinst, guis[ifce].clickEventHandler,
                           // (int)&scrGui[ifce], mbut);`. This build's own `process_interface_click`
                           // (already matched) has NO such branch at all: its very first
                           // instructions unconditionally decode `objrefptr[btn]` with no `btn<0`
                           // check anywhere in the function, and its caller (`process_event`,
                           // already matched) pushes only 2 arguments (`ifce`/`btn`) for this call
                           // -- not the 3 (`ifce`/`btn`/`mbut`) 2011's signature needs -- confirmed
                           // by the matching `add esp,8` cleanup immediately after the call site.
                           // `mbut` is never referenced anywhere in the function body either. This
                           // build's version of the function is a genuine 2-argument predecessor
                           // that predates the "click GUI background triggers clickEventHandler"
                           // feature entirely -- not an unused code path, a never-compiled one. See
                           // `process_interface_click`'s own matches.json entry for the complete
                           // writeup, including the newly-matched `run_text_script_2iparam`
                           // (`sub_409F23`) this finding was confirmed against. FURTHER SUPPORTING
                           // EVIDENCE (found later, same struct-level round as `vtext` above): the
                           // newly-found `GUIMain::init()`-equivalent zeroes `vtext[0]`@+0x00
                           // explicitly but does NOT zero `clickEventHandler[0]`@+0x14 -- 2011's own
                           // constructor does BOTH ("vtext[0]=0; clickEventHandler[0]=0;",
                           // `acgui.cpp:987-988`, back to back). This asymmetry -- one adjacent
                           // char-array zero present, the very next one absent -- is a second,
                           // independent piece of evidence (on top of `process_interface_click`'s
                           // total lack of a reader) consistent with `clickEventHandler` not
                           // existing as a distinct field in this build at all, though it isn't
                           // fully decisive on its own (a compiler/source could in principle zero
                           // one and not the other for unrelated reasons).
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
  int focus;                 // +0x38, high confidence (UPGRADED from MEDIUM): confirmed via a
                           // newly-found `GUIMain::init()`-equivalent (see `bgcol`'s own entry
                           // below for the complete writeup and its important IDA-boundary
                           // caveat): "[this+0x38]=0" matches source's "focus=0;" (`acgui.cpp:989`)
                           // exactly, in the same relative position as source's assignment order.
                           // 2011 itself has ZERO usages of `focus` anywhere in `Engine/` past this
                           // one constructor default (checked this round) -- genuinely vestigial
                           // even in 2011, so no further LIVE-usage confirmation is expected to be
                           // findable in either build.
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
  int bgcol;                  // +0x48, high confidence (UPGRADED from MEDIUM): confirmed via a
                           // newly-found `GUIMain::init()`-equivalent, discovered this round while
                           // chasing `bgcol` (2011's own reader for it in `adjust_x_for_guis`/
                           // `adjust_y_for_guis` had already been shown absent, and `wbar` -- the
                           // Allegro rectangle-fill call `draw_gui_for_dialog_options`'s bgcol path
                           // needs -- has ZERO occurrences anywhere in this entire binary,
                           // confirming that specific dialog-options-GUI rendering path absent too;
                           // this constructor lead paid off where those didn't). Located immediately
                           // after `sub_407360`'s `endp` (itself a `vector constructor iterator`
                           // wrapper for the static `GUIListBox` array, `unk_4D31D0`,
                           // ElementSize=0x1DC/Count=0x51) and immediately before
                           // `GUIMain__rebuild_array`'s own `proc` -- part of the same C++ static-
                           // array-construction chain rooted at `sub_407356` (itself referenced via
                           // a DATA XREF from `.data`, consistent with an MSVC global-static-
                           // initializer table entry, i.e. this whole chain runs automatically
                           // before `main()`, not from an explicit call site). IMPORTANT CAVEAT:
                           // this code has NO formal IDA function boundary (no `proc`/`endp`, no
                           // visible name or CODE XREF) -- it reads as loose instructions between
                           // two properly-defined functions, so it cannot be given a `matches.json`
                           // function-match entry the normal way (`apply_matches.py` resolves
                           // `asm_name` via `idc.get_name_ea_simple`, which needs an existing name
                           // to look up). A human needs to define the function in IDA (Alt-P at its
                           // start address) before it can be formally renamed; until then this is
                           // recorded as pure field evidence, not a function match. Content is an
                           // exact, near-complete match to source's `GUIMain::init()`
                           // (`acgui.cpp:985-1000`): "[this+0x38]=0; [this+0x3C]=0; [this+0x54]=-1;
                           // [this+0x58]=-1; [this+0x5C]=-1; [this+0x60]=-1; [this+0x64]=-1;
                           // [this+0x90]=1; [this+0x50]=1; [this+0x48]=8; [this+0x68]=0" (preceded
                           // by the single-byte `vtext[0]=0` write, see that field's own entry) --
                           // matching source's "focus=0; numobjs=0; mouseover=-1; mousewasx=-1;
                           // mousewasy=-1; mousedownon=-1; highlightobj=-1; on=1; fgcol=1; bgcol=8;
                           // flags=0;" line for line, VALUE for VALUE, across 11 of source's 12
                           // assignments (only `clickEventHandler[0]=0` is missing -- see that
                           // field's own entry for the drift this implies). This single find closes
                           // or reconfirms nearly every remaining `GUIMain` field at once: `bgcol`
                           // here, `focus`/`mousewasx`/`mousewasy`/`highlightobj` at their own
                           // entries, and `numobjs`/`mouseover`/`mousedownon`/`on`/`fgcol`/`flags`
                           // all independently reconfirmed (the `fgcol=1` default is a clean
                           // cross-check against this SAME round's separate `wtextcolor`-based
                           // confirmation of `fgcol`@+0x50).
  int bgpic;                  // +0x4C, high confidence: confirmed via `SetGUIBackgroundPic` (already
                           // matched, script-exported): "mov [guis+guin*184h+4Ch], slotn" -- sets
                           // this field directly from its `slotn` parameter, matching 2011's
                           // declared field in position exactly.
  int fgcol;                  // +0x50, high confidence (UPGRADED from MEDIUM): confirmed via
                           // `_display_main` (already matched): an inlined custom-text-window-GUI
                           // branch does "push [guis+ifnum*184h+50h]; call sub_401F62" -- matching
                           // source's "wtextcolor(guis[ifnum].fgcol);" (`Engine/AC.CPP:12931`,
                           // inside `draw_text_window_and_bar`'s speech-GUI branch, inlined here
                           // rather than called separately) exactly, where `ifnum` is this build's
                           // already-confirmed `GameState.speech_textwindow_gui` global (see
                           // `main`'s own matches.json entry) getting a new reader. `sub_401F62`
                           // itself is independently, decisively confirmed as `wtextcolor` via
                           // `GUILabel__Draw` (already matched): "push [this+0xEC]; call
                           // sub_401F62" matches source's "wtextcolor(textcol);" (`acgui.cpp:354`)
                           // exactly, where `[this+0xEC]` is `GUILabel.textcol`, already
                           // independently confirmed via `GUILabel__ReadFromFile`'s own default-
                           // value logic.
  int mouseover;          // +0x54, confirmed via GUIMain::mouse_but_down
  int mousewasx;              // +0x58, high confidence (UPGRADED from MEDIUM): confirmed via the
                           // same newly-found `GUIMain::init()`-equivalent as `bgcol` (see its own
                           // entry for the complete writeup and caveat): "[this+0x58]=-1" matches
                           // source's "mousewasx=-1;" (`acgui.cpp:992`) exactly.
  int mousewasy;              // +0x5C, high confidence (UPGRADED from MEDIUM): same source, "
                           // [this+0x5C]=-1" matches "mousewasy=-1;" (`acgui.cpp:993`) exactly --
                           // see `bgcol`'s own entry for the complete writeup.
  int mousedownon;        // +0x60, confirmed via GUIMain::mouse_but_up/down. RECONFIRMED via the
                           // same `GUIMain::init()`-equivalent: "[this+0x60]=-1" matches
                           // "mousedownon=-1;" (`acgui.cpp:994`) exactly.
  int highlightobj;           // +0x64, high confidence (UPGRADED from MEDIUM): confirmed via the
                           // same newly-found `GUIMain::init()`-equivalent as `bgcol` (see its own
                           // entry for the complete writeup and caveat): "[this+0x64]=-1" matches
                           // source's "highlightobj=-1;" (`acgui.cpp:995`) exactly.
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
                           // not yet wired up to anything that reads or writes it. FURTHER
                           // SUPPORTING EVIDENCE (this round): `read_gui` (already matched)'s own
                           // per-GUI post-load loop -- fully read this round -- matches 2011's
                           // `hit<2` clamp (`acgui.cpp:1478-1480`) exactly, but then calls
                           // `GUIMain__rebuild_array` immediately, with NO version-gated
                           // `"if(gver<105) zorder=ee;"` (`acgui.cpp:1484-1485`) default assignment
                           // in between -- the entire version-gating block for this era of the GUI
                           // file format (`gver<103`/`gver<105` checks) is simply absent from this
                           // build's loop. Consistent with, not just circumstantial alongside,
                           // `GetGUIAt`'s already-noted lack of `gui_draw_order[]` indirection.
                           // DATED (via `ags-archives/`, see `reversing/notes/
                           // ags-archives-cross-reference.md`): `ags261/docs/CHANGES.TXT`'s
                           // "VERSION 2.6, December 2003" entry adds `SetGUIZOrder` and "GUI
                           // Z-order support, so that you can choose which order overlapping GUIs
                           // are drawn in" -- confirming this feature (and, in the same version
                           // entry, `SetGUIClickable`/`SetGUIObjectSize`, both also independently
                           // searched-for-and-absent elsewhere in this project) postdates Rob
                           // Blanc 1's own ~2.4b (July 2002) era by well over a year.
  int guiId;                  // +0x74, MEDIUM confidence: positional/arithmetic fit only, matching
                           // 2011's declared field. CHECKED THIS ROUND, still not independently
                           // confirmed: `read_gui`'s per-GUI post-load loop (see `zorder` above,
                           // fully read this round) never performs 2011's unconditional
                           // `guiread[ee].guiId=ee;` (`acgui.cpp:1487`) -- the loop goes straight
                           // from the `hit<2` clamp to calling `GUIMain__rebuild_array`, and that
                           // function's own body (also fully read this round -- see its own
                           // matches.json entry) doesn't read `this->guiId` either, unlike 2011's
                           // `objs[ff]->guin=this->guiId;` (`acgui.cpp:1128`). Both of `guiId`'s
                           // only 2011 write/read sites are confirmed absent from their exact
                           // disassembly counterparts -- a real negative result, though not (yet)
                           // the exhaustive whole-binary search this project's "confirmed absent"
                           // standard requires.
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
  int talkview;              // +0x04, high confidence (UPGRADED from TENTATIVE, found in a later
                           // round): `run_dialog_script` (this build's dialog-script byte-code
                           // interpreter, corrected from an earlier `run_dialog_request`
                           // misidentification -- see its own matches.json entry)'s `DCMD_
                           // SETSPCHVIEW`(11) opcode handler does "eax=viewnum-1;
                           // game_chars[charID*0x140+4]=eax" -- writing a (1-based-to-0-based
                           // converted) view number directly into this exact offset, matching
                           // 2011's still-declared `DCMD_SETSPCHVIEW` role ("SetCharacterSpeechView"
                           // -- change talkview) and confirming the original positional-adjacency
                           // guess (`defview`@+0x00, `view`@+0x08, `room`@+0x0C) was correct all
                           // along. INDEPENDENTLY CROSS-CONFIRMED (found via the newly-added
                           // `ags-archives/` resource -- see `reversing/notes/
                           // ags-archives-cross-reference.md` for the complete writeup): Chris
                           // Jones's own official `.CHA` character-file format documentation
                           // (`ags-archives/ags240/docs/TECHINFO.TXT` section 1.2, dated 26
                           // December 2001 -- i.e. contemporary with Rob Blanc 1's own era, not a
                           // 9-years-later reference) declares "+04h DWORD talking view number" at
                           // this EXACT offset. This document also independently pins `defview`@
                           // +0x00/`view`@+0x08/`room`@+0x0C, and its "+44h 204BYTEs [internal],
                           // then 30-byte name, then 16-byte scriptname" arithmetic lands EXACTLY
                           // on this project's own already-confirmed `name`@+0x110 and
                           // `scrname`@+0x12E with zero slack -- four independently-confirmed
                           // fields agreeing with an authoritative period document, about as
                           // strong a cross-validation as this project has had.
  int view;                // +0x08, high confidence, SUPERSEDES an earlier "wait" guess (see
                           // reversing/notes/struct-layout-drift.md for the retraction and why).
                           // update_stuff uses this as `imul ecx, 8D4h` array index into a per-view
                           // data table -- an unambiguous "this is a view number" pattern. Also directly
                           // confirmed again via Character_UnlockView's defview assignment (see above).
  int room;                // +0x0C, high confidence: SetPlayerCharacter saves this, switches
                           // playerchar, and calls NewRoom(new playerchar->room) if it changed --
                           // matches source exactly.
  int prevroom;              // +0x10, high confidence (UPGRADED from TENTATIVE, found in a later
                           // round while chasing the `ags-archives/`-confirmed field name -- see
                           // `reversing/notes/ags-archives-cross-reference.md`): `load_new_room`
                           // (already matched) does "mov eax,[forchar+0Ch]; mov [forchar+10h],eax;
                           // mov edx,[newnum]; mov [forchar+0Ch],edx" -- a direct, complete,
                           // multi-instruction match to source's "forchar->prevroom=forchar->room;
                           // forchar->room=newnum;" (`Engine/AC.CPP:4431-4432`), including the
                           // identical `offsetx=0; offsety=0;` initialization immediately
                           // preceding it in both. `ags240/docs/CHANGES.TXT`'s own 2.15 entry
                           // ("Fixed prevroom text script variable for following characters") had
                           // already confirmed `prevroom` as a genuine, officially-named AGS field
                           // from this exact era; this round supplies the missing disassembly-side
                           // confirmation of its exact byte offset.
  int x;                   // +0x14, high confidence: exact arg-order match in get_hotspot_at's
                           // caller (mainloop), matching source's get_hotspot_at(playerchar->x, playerchar->y).
                           // Matches `ags240/docs/TECHINFO.TXT`'s own documented "+14h X-coordinate"
                           // exactly (see `talkview`'s own entry above for the complete cross-
                           // reference writeup).
  int y;                   // +0x18, see x above. Matches TECHINFO.TXT's documented "+18h
                           // Y-coordinate" exactly.
  int wait;                 // +0x1C, high confidence, RESOLVES the round-2 "decrement-if-positive
                           // countdown at +0x1C" lead (which round-2 mis-attributed to +0x08, since
                           // corrected to "view"): Character_LockView (Engine/acchars.cpp:824, what
                           // SetCharacterView delegates to) does "chap->wait=0;" -- matches [+0x1C]=0
                           // exactly. update_stuff's "if (chi->wait>0) chi->wait--;" lip-sync decrement
                           // pattern (originally spotted in round 2) belongs here, not at +0x08.
                           // MAJOR ADDITIONAL ROLE (CharacterExtras remaining-fields round): this
                           // SAME field ALSO serves as 2011's `walkwait` (TURNING_AROUND-branch
                           // "if (chi->walkwait>0) chi->walkwait--;", AC.CPP:6528) AND `charextra[].
                           // animwait` (the "walking<1 -> animwait=0", "animwait>0 -> animwait--",
                           // and final "animwait=views[...].frames[...].speed+chi->animspeed" trio,
                           // AC.CPP:6626-6653) -- all three read/write this one field, matching
                           // `OldCharacterInfo`'s single declared `wait` (`acroom.h:2604`, no
                           // separate `walkwait` in this ancestor at all). CONFIRMS `charextra[].
                           // animwait` is not a separate field in this build -- see CharacterExtras's
                           // own documentation block below for the full writeup.
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
  short loop;               // +0x38, high confidence (UPGRADED from MEDIUM, found in a later
                           // round): `update_stuff`'s "turning around before walking" branch
                           // (gated on `walking>=0x3E8`/`TURNING_AROUND`, already-confirmed
                           // `walking`@+0x3C) is a complete, decisive, multi-field match to
                           // source's `AC.CPP:6526-6558`: reads `loop`@+0x38 as the sole argument
                           // to a newly-identified helper (`sub_40EB43`, now `find_looporder_index`
                           // -- matching `find_looporder_index(chi->loop)` exactly), clamps the
                           // resulting `wantloop` to [0,7], validates it against
                           // `views[view].numLoops`/`CHF_NODIAGONAL`(8, already-confirmed
                           // `flags`@+0x20) using a newly-identified global 8-entry table
                           // (`dword_4B42C8`, now identified as `turnlooporder[8]={0,6,1,7,3,5,2,4}`
                           // itself), and finally writes "dx=word[dword_4B42C8[wantloop*4]];
                           // [this+0x38]=dx" -- `chi->loop=turnlooporder[wantloop];` -- as the
                           // SOLE, unambiguous write target. The same block goes on to decrement
                           // `walking` by `TURNING_AROUND` and take it modulo `TURNING_BACKWARDS`
                           // (0x2710/10000, matching this project's own earlier "modular-1000/
                           // 10000 arithmetic" observation on `walking` exactly) and copy
                           // `animspeed`@+0x42 into `wait`@+0x1C (`chi->walkwait=chi->animspeed;`)
                           // -- reconfirming FOUR other already-established fields in the same
                           // pass.
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
                           // First confirmed bit value (found via `animate_character`, newly
                           // documented this round): "animating=1; if(rept) animating|=2;" matches
                           // 2011's `CHANIM_REPEAT=2` (`Common/acruntim.h:810`) exactly.
  short walkspeed;          // +0x40, high confidence: walk_character reads this into a global right where
                           // source has "int move_speed_x = chin->walkspeed;" (the very next source line
                           // after the animating check above).
  short animspeed;          // +0x42, high confidence (UPGRADED from tentative, CharacterExtras
                           // remaining-fields round): `update_stuff` (already matched) reads this
                           // field directly as the second addend in "wait =
                           // views[chi->view].loops[chi->loop].frames[chi->frame].speed +
                           // chi->animspeed;", matching AC.CPP:6653 exactly -- see `wait`@+0x1C's
                           // own comment above for the full context.
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
                            // importsCapacity-sized allocation). REINFORCED (found this round,
                            // `ccFreeScript` -- newly matched, `RoomStruct.compiled_script`'s own
                            // destructor): this build's destructor frees each individual
                            // `imports[]`/`exports[]` ELEMENT but never the arrays `imports`/
                            // `exports`/`export_addr` themselves -- unlike 2011's own
                            // `ccFreeScript` (`Common/cscommon.cpp:160-167`), which explicitly
                            // `free()`s all three array pointers right after. Consistent with
                            // there being nothing TO free here (fixed embedded arrays, not
                            // separately-`malloc`'d ones) -- independent confirmation of the
                            // drift already inferred from the length arithmetic above.
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
  int zorder;                  // +0x18, high confidence (UPGRADED from an unconfirmed pad, found
                            // this round while surveying GUIMain's own now-mostly-closed field
                            // list and pivoting to the shared GUIObject base): 2011's own
                            // `GUIObject::WriteToFile`/`ReadFromFile` (`Engine/acgui.cpp:69-83`)
                            // read/write the base class as ONE bulk block, "fread(&flags,
                            // sizeof(int), BASEGOBJ_SIZE, ooo);" with `BASEGOBJ_SIZE=7`
                            // (`Common/acgui.h:119`) -- exactly 7 consecutive ints starting at
                            // `flags`. This build's own confirmed layout already has `flags`@+0x04
                            // through `activated`@+0x1C occupying exactly that same 7-int/28-byte
                            // span (already-matched `ReadFromFile`/`WriteToFile` methods for all
                            // six derived classes independently confirm both endpoints, `flags` and
                            // `activated`, at their own exact offsets) -- meaning this position MUST
                            // hold a real field, not padding, for the bulk-block argument to hold at
                            // all. 2011's declared order for this exact span (`Common/acgui.h:
                            // 128-133`, `flags,x,y,wid,hit,zorder,activated`) has only one field
                            // between `hit` and `activated`: `zorder` -- this build's own
                            // per-CONTROL z-order (distinct from `GUIMain`'s already-shown-unused
                            // per-GUI z-order; 2011's per-control equivalent, `resort_zorder()`, is
                            // similarly never called from this build's `GUIMain__rebuild_array`, so
                            // this field is plausibly present-but-inert here too, matching the same
                            // pattern rather than contradicting it). Applies identically to all six
                            // `GUIObject`-derived structs in this file (`GUIButton`/`GUISlider`/
                            // `GUILabel`/`GUITextBox`/`GUIListBox`/`GUIInv`), which all share this
                            // exact base-class layout.
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
  // Total confirmed size 0x84 (132 bytes), HIGH confidence (UPGRADED from an explicitly-flagged
  // "minimum/partial size" caveat): reading GUIButton::ReadFromFile/WriteToFile (sub_406A9C/
  // sub_406A4A, both already matched) in full end to end -- not just the three fread/fwrite calls
  // already used to recover the fields above -- shows each function does EXACTLY those three
  // calls (28 bytes@+0x04, 48 bytes@+0x54, 50 bytes@+0x20) and then returns immediately (a small
  // textcol-default fixup in ReadFromFile, nothing else). No fourth call, no further offset past
  // +0x80 is ever touched by either function. This positively confirms 2011's trailing
  // `textAlignment`/`reserved1`/`eventHandlers[]` fields (`Common/acgui.h`, declared after
  // `rclickdata`) are CONFIRMED ABSENT here, not just unconfirmed -- this build's save-file
  // format for GUIButton genuinely ends at `rclickdata`. sizeof(GUIButton) == 0x84 confirmed.
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
  int zorder;                  // +0x18, high confidence (UPGRADED from an unconfirmed pad, found
                            // this round while surveying GUIMain's own now-mostly-closed field
                            // list and pivoting to the shared GUIObject base): 2011's own
                            // `GUIObject::WriteToFile`/`ReadFromFile` (`Engine/acgui.cpp:69-83`)
                            // read/write the base class as ONE bulk block, "fread(&flags,
                            // sizeof(int), BASEGOBJ_SIZE, ooo);" with `BASEGOBJ_SIZE=7`
                            // (`Common/acgui.h:119`) -- exactly 7 consecutive ints starting at
                            // `flags`. This build's own confirmed layout already has `flags`@+0x04
                            // through `activated`@+0x1C occupying exactly that same 7-int/28-byte
                            // span (already-matched `ReadFromFile`/`WriteToFile` methods for all
                            // six derived classes independently confirm both endpoints, `flags` and
                            // `activated`, at their own exact offsets) -- meaning this position MUST
                            // hold a real field, not padding, for the bulk-block argument to hold at
                            // all. 2011's declared order for this exact span (`Common/acgui.h:
                            // 128-133`, `flags,x,y,wid,hit,zorder,activated`) has only one field
                            // between `hit` and `activated`: `zorder` -- this build's own
                            // per-CONTROL z-order (distinct from `GUIMain`'s already-shown-unused
                            // per-GUI z-order; 2011's per-control equivalent, `resort_zorder()`, is
                            // similarly never called from this build's `GUIMain__rebuild_array`, so
                            // this field is plausibly present-but-inert here too, matching the same
                            // pattern rather than contradicting it). Applies identically to all six
                            // `GUIObject`-derived structs in this file (`GUIButton`/`GUISlider`/
                            // `GUILabel`/`GUITextBox`/`GUIListBox`/`GUIInv`), which all share this
                            // exact base-class layout.
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
  int zorder;                  // +0x18, high confidence (UPGRADED from an unconfirmed pad, found
                            // this round while surveying GUIMain's own now-mostly-closed field
                            // list and pivoting to the shared GUIObject base): 2011's own
                            // `GUIObject::WriteToFile`/`ReadFromFile` (`Engine/acgui.cpp:69-83`)
                            // read/write the base class as ONE bulk block, "fread(&flags,
                            // sizeof(int), BASEGOBJ_SIZE, ooo);" with `BASEGOBJ_SIZE=7`
                            // (`Common/acgui.h:119`) -- exactly 7 consecutive ints starting at
                            // `flags`. This build's own confirmed layout already has `flags`@+0x04
                            // through `activated`@+0x1C occupying exactly that same 7-int/28-byte
                            // span (already-matched `ReadFromFile`/`WriteToFile` methods for all
                            // six derived classes independently confirm both endpoints, `flags` and
                            // `activated`, at their own exact offsets) -- meaning this position MUST
                            // hold a real field, not padding, for the bulk-block argument to hold at
                            // all. 2011's declared order for this exact span (`Common/acgui.h:
                            // 128-133`, `flags,x,y,wid,hit,zorder,activated`) has only one field
                            // between `hit` and `activated`: `zorder` -- this build's own
                            // per-CONTROL z-order (distinct from `GUIMain`'s already-shown-unused
                            // per-GUI z-order; 2011's per-control equivalent, `resort_zorder()`, is
                            // similarly never called from this build's `GUIMain__rebuild_array`, so
                            // this field is plausibly present-but-inert here too, matching the same
                            // pattern rather than contradicting it). Applies identically to all six
                            // `GUIObject`-derived structs in this file (`GUIButton`/`GUISlider`/
                            // `GUILabel`/`GUITextBox`/`GUIListBox`/`GUIInv`), which all share this
                            // exact base-class layout.
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
  int zorder;                  // +0x18, high confidence (UPGRADED from an unconfirmed pad, found
                            // this round while surveying GUIMain's own now-mostly-closed field
                            // list and pivoting to the shared GUIObject base): 2011's own
                            // `GUIObject::WriteToFile`/`ReadFromFile` (`Engine/acgui.cpp:69-83`)
                            // read/write the base class as ONE bulk block, "fread(&flags,
                            // sizeof(int), BASEGOBJ_SIZE, ooo);" with `BASEGOBJ_SIZE=7`
                            // (`Common/acgui.h:119`) -- exactly 7 consecutive ints starting at
                            // `flags`. This build's own confirmed layout already has `flags`@+0x04
                            // through `activated`@+0x1C occupying exactly that same 7-int/28-byte
                            // span (already-matched `ReadFromFile`/`WriteToFile` methods for all
                            // six derived classes independently confirm both endpoints, `flags` and
                            // `activated`, at their own exact offsets) -- meaning this position MUST
                            // hold a real field, not padding, for the bulk-block argument to hold at
                            // all. 2011's declared order for this exact span (`Common/acgui.h:
                            // 128-133`, `flags,x,y,wid,hit,zorder,activated`) has only one field
                            // between `hit` and `activated`: `zorder` -- this build's own
                            // per-CONTROL z-order (distinct from `GUIMain`'s already-shown-unused
                            // per-GUI z-order; 2011's per-control equivalent, `resort_zorder()`, is
                            // similarly never called from this build's `GUIMain__rebuild_array`, so
                            // this field is plausibly present-but-inert here too, matching the same
                            // pattern rather than contradicting it). Applies identically to all six
                            // `GUIObject`-derived structs in this file (`GUIButton`/`GUISlider`/
                            // `GUILabel`/`GUITextBox`/`GUIListBox`/`GUIInv`), which all share this
                            // exact base-class layout.
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
  int zorder;                  // +0x18, high confidence (UPGRADED from an unconfirmed pad, found
                            // this round while surveying GUIMain's own now-mostly-closed field
                            // list and pivoting to the shared GUIObject base): 2011's own
                            // `GUIObject::WriteToFile`/`ReadFromFile` (`Engine/acgui.cpp:69-83`)
                            // read/write the base class as ONE bulk block, "fread(&flags,
                            // sizeof(int), BASEGOBJ_SIZE, ooo);" with `BASEGOBJ_SIZE=7`
                            // (`Common/acgui.h:119`) -- exactly 7 consecutive ints starting at
                            // `flags`. This build's own confirmed layout already has `flags`@+0x04
                            // through `activated`@+0x1C occupying exactly that same 7-int/28-byte
                            // span (already-matched `ReadFromFile`/`WriteToFile` methods for all
                            // six derived classes independently confirm both endpoints, `flags` and
                            // `activated`, at their own exact offsets) -- meaning this position MUST
                            // hold a real field, not padding, for the bulk-block argument to hold at
                            // all. 2011's declared order for this exact span (`Common/acgui.h:
                            // 128-133`, `flags,x,y,wid,hit,zorder,activated`) has only one field
                            // between `hit` and `activated`: `zorder` -- this build's own
                            // per-CONTROL z-order (distinct from `GUIMain`'s already-shown-unused
                            // per-GUI z-order; 2011's per-control equivalent, `resort_zorder()`, is
                            // similarly never called from this build's `GUIMain__rebuild_array`, so
                            // this field is plausibly present-but-inert here too, matching the same
                            // pattern rather than contradicting it). Applies identically to all six
                            // `GUIObject`-derived structs in this file (`GUIButton`/`GUISlider`/
                            // `GUILabel`/`GUITextBox`/`GUIListBox`/`GUIInv`), which all share this
                            // exact base-class layout.
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
  int zorder;                  // +0x18, high confidence (UPGRADED from an unconfirmed pad, found
                            // this round while surveying GUIMain's own now-mostly-closed field
                            // list and pivoting to the shared GUIObject base): 2011's own
                            // `GUIObject::WriteToFile`/`ReadFromFile` (`Engine/acgui.cpp:69-83`)
                            // read/write the base class as ONE bulk block, "fread(&flags,
                            // sizeof(int), BASEGOBJ_SIZE, ooo);" with `BASEGOBJ_SIZE=7`
                            // (`Common/acgui.h:119`) -- exactly 7 consecutive ints starting at
                            // `flags`. This build's own confirmed layout already has `flags`@+0x04
                            // through `activated`@+0x1C occupying exactly that same 7-int/28-byte
                            // span (already-matched `ReadFromFile`/`WriteToFile` methods for all
                            // six derived classes independently confirm both endpoints, `flags` and
                            // `activated`, at their own exact offsets) -- meaning this position MUST
                            // hold a real field, not padding, for the bulk-block argument to hold at
                            // all. 2011's declared order for this exact span (`Common/acgui.h:
                            // 128-133`, `flags,x,y,wid,hit,zorder,activated`) has only one field
                            // between `hit` and `activated`: `zorder` -- this build's own
                            // per-CONTROL z-order (distinct from `GUIMain`'s already-shown-unused
                            // per-GUI z-order; 2011's per-control equivalent, `resort_zorder()`, is
                            // similarly never called from this build's `GUIMain__rebuild_array`, so
                            // this field is plausibly present-but-inert here too, matching the same
                            // pattern rather than contradicting it). Applies identically to all six
                            // `GUIObject`-derived structs in this file (`GUIButton`/`GUISlider`/
                            // `GUILabel`/`GUITextBox`/`GUIListBox`/`GUIInv`), which all share this
                            // exact base-class layout.
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
  char name[25];                  // +0x00, high confidence (UPGRADED from an unconfirmed pad,
                            // found in a later round): `GetInvName` (already matched,
                            // script-exported, previously only mechanically linker-matched with no
                            // field evidence recorded) does "imul eax,44h /*this struct's own
                            // confirmed stride*/; add eax,offset byte_51B854; push eax; call
                            // GetTranslation; ...; strcpy(Destination,result);" -- reading
                            // `invinfo[index].name` starting at THIS struct's own base address
                            // (`byte_51B854`, offset 0 within the element) and passing it straight
                            // to `GetTranslation`, matching 2011's `GetInvName`
                            // ("strcpy(usebuf,get_translation(game.invinfo[indx].name));") almost
                            // certainly verbatim. Cross-confirms `byte_51B854` as `invinfo[]`'s own
                            // base address a further way: `byte_51B854+0x1C` lands EXACTLY on the
                            // already-confirmed `pic`@+0x1C's own address (`dword_51B870`), zero
                            // slack.
  char _pad_align1[3];            // +0x19..0x1C, compiler alignment padding (not a real field) --
                            // boxed in with zero slack between `name`'s own confirmed 25-byte
                            // extent and `pic`'s own confirmed start.
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
  char _unconfirmed2[4];          // +0x20..0x24 (4 bytes), CONFIRMED ABSENT (UPGRADED from an
                            // "unconfirmed gap, no negative evidence either way" caveat): 2011's
                            // own `set_inv_item_pic` (`Engine/AC.CPP:5262-5278`) carries an
                            // explicit, decisive comment right at the point that would distinguish
                            // the two builds -- "if (game.invinfo[invi].pic ==
                            // game.invinfo[invi].cursorPic) { // Backwards compatibility -- there
                            // didn't used to be a cursorPic, so if they're the same update both.
                            // set_inv_item_cursorpic(invi, piccy); }" -- i.e. 2011's own source
                            // directly documents that `cursorPic` is a LATER addition, kept in
                            // sync with `pic` only for save-compatibility with exactly this era.
                            // This build's own `SetInvItemPic` (already matched, re-read this
                            // round end to end) matches the PRE-cursorPic shape the comment
                            // describes: it does ONE unconditional write,
                            // "dword_51B870[ecx]=piccy" (`pic`@+0x1C only), with no `pic==piccy`
                            // early-return check and no second-field sync branch at all --
                            // simpler than even 2011's own "backwards compatibility" branch,
                            // consistent with never having had a second field to sync in the
                            // first place. Reinforced by exhaustive search: no
                            // `set_inv_item_cursorpic`/`InventoryItem::SetCursorGraphic`/
                            // `InventoryItem::GetCursorGraphic`-equivalent function, and no
                            // `"SetCursorGraphic"`-family export string, exists anywhere in this
                            // binary. `cursorPic` is confirmed absent; `pic` alone serves both
                            // roles in this build.
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
                            // THIRD confirmation (found this round): `DisplayMessage` (already
                            // matched)'s own "global message" branch (`msnum>=500`) reads
                            // `dword_51CB50[msnum]` directly (no `-500` subtraction visible in
                            // the disassembly) -- decisively explained by `dword_51CB50 ==
                            // dword_51D320 - 500*4` (`0x51D320-0x51CB50=0x7D0=2000=500*4`,
                            // zero slack), i.e. the compiler folded the `messages[msnum-500]`
                            // indexing into a single pre-adjusted base address, matching 2011's
                            // "if((msnum>=MAXGLOBALMES+500)||(game.messages[msnum-500]==NULL))
                            // quit(\"!DisplayGlobalMessage: message does not exist\");" (`AC.CPP:
                            // 14257-14259`) exactly -- the same "IDA doesn't recognize the two
                            // addresses as the same array" pattern already seen with `ebscene[]`/
                            // `dword_523094` a few rounds ago.
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
                            // the MAJOR FINDING note below). DATED (via `ags-archives/`, see
                            // `reversing/notes/ags-archives-cross-reference.md`): `ags240/docs/
                            // CHANGES.TXT`'s own "VERSION 2.4, July 2002" entry says "Increase
                            // limit to 6000 sprite slots, 300 views" -- an exact match to this
                            // build's own independently-confirmed 6000 capacity, pinning Rob Blanc
                            // 1's engine version to `>= 2.4` (July 2002) from a completely
                            // different angle than the `IsMusicVoxAvailable`/OGG-absence dating.
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
                            // zeroes this offset first. FURTHER RECONFIRMATION (found later,
                            // `run_text_script_2iparam`'s own entry): a single dereference of the
                            // newly-identified global `curscript` (`dword_52314C`, this build's
                            // `ExecutingScript *curscript;`) supplies `ccCallInstance`'s first
                            // argument with no added offset, matching source's
                            // `ccCallInstance(curscript->inst,...)` (`Engine/AC.CPP:3281`) exactly
                            // -- a third independent confirmation that `inst` is genuinely the
                            // struct's first field.
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
  // other struct in this project, `Common/acroom.h` preserves NO older ANCESTOR declaration for
  // `DialogTopic` to check this against -- 2011's own `DialogTopic` is ~4696 bytes
  // (`optionnames[MAXTOPICOPTIONS=30][150]` alone is 4500), over 4x bigger than this build's
  // confirmed total. This was a genuinely novel, from-scratch reconstruction when first tackled,
  // not a "confirm against a known reference" exercise like every other struct in this project --
  // though a SUCCESSOR-era declaration was later found via `ags-archives/` (see
  // `reversing/notes/ags-archives-cross-reference.md`): `ags-archives/ags261/docs/TECHINFO.TXT`
  // (dated 12 April 2004, so documenting the LATER 30-option/150-char layout, not this build's own
  // 15/70 one directly) declares `struct DialogTopic { char optionNames[30][150]; DWORD
  // optionFlags[30]; DWORD hasCompiledScript; WORD entryPoints[30]; WORD startupEntryPoint; WORD
  // codeSize; DWORD numOptions; DWORD topicFlags; };` -- matching this build's own independently-
  // confirmed field ORDER and TYPES exactly (optionflags as 4-byte ints, entrypoints/
  // startupentrypoint/codesize as 2-byte shorts, numoptions as an int), with `topicFlags` the one
  // trailing field confirmed absent here (the struct ends exactly at `numoptions`) -- a
  // contemporary-lineage independent confirmation of this project's own from-scratch layout, even
  // though it postdates Rob Blanc 1 itself. The global
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
  short entrypoints[15];            // +0x45C..0x47A (30 bytes), high confidence (UPGRADED from
                            // MEDIUM): confirmed via `do_conversation` (already matched):
                            // "movsx edx,word[parmtr+var_4C*2+0x45C]; push edx; push parmtr; call
                            // <bytecode interpreter>" -- reading `dtop->entrypoints[chose]` and
                            // passing it as the start-offset argument into the dialog-script
                            // bytecode interpreter, matching 2011's
                            // "run_dialog_script(dtop,dlgnum,dtop->entrypoints[chose],chose+1);"
                            // (`AC.CPP:22564`) in ROLE exactly (this build's own 2-argument
                            // predecessor of that call only needs `dtpp`/`offse` -- see the
                            // interpreter's own entry, previously misidentified as
                            // `run_dialog_request`, now corrected to `run_dialog_script`, for the
                            // complete writeup). 15 elements at 2 bytes each matches
                            // `MAXTOPICOPTIONS=15`, already confirmed THREE other independent ways
                            // for this struct (`optionnames`, `optionflags`, and the
                            // `SaveGameSlot`/`restore_game_data` literal `0x0F` constant).
  short startupentrypoint;         // +0x47A, high confidence: confirmed via `do_conversation`
                            // (already matched): "movsx edx,word[parmtr+0x47A]; push edx; push
                            // parmtr; call <bytecode interpreter>" -- passed alongside the dialog
                            // topic pointer itself to this build's dialog-script bytecode
                            // interpreter (previously misidentified as `run_dialog_request`; see
                            // that function's own entry, now corrected to `run_dialog_script`, for
                            // the complete writeup), matching 2011's semantic role ("initial
                            // dialog-script entry point to jump to") and position exactly. This is
                            // do_conversation's OWN opening call into the interpreter, a SECOND
                            // independent confirmation of the interpreter's 2-argument call shape
                            // alongside `entrypoints[]`'s own confirming call site.
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
  int xpermove[40];                 // +0x0A4..0x144 (160 bytes), high confidence (UPGRADED from
                            // MEDIUM, found in a later round): `do_movelist_move` (already
                            // matched, `Engine/AC.CPP:17327`, this build's per-frame move-list
                            // consumer): "edx=cmls[+0x1EC] /*onstage*/; ecx=cmls[edx*4+0xA4]" --
                            // an `onstage`-indexed read, matching source's "fixed
                            // xpermove=cmls->xpermove[cmls->onstage];" (`AC.CPP:17331`) exactly.
                            // Matches 2011's declared `fixed xpermove[MAXNEEDSTAGES]` (`fixed` is
                            // a 4-byte fixed-point int typedef, `acroom.h:3085`) in position, size,
                            // AND now behavior.
  int ypermove[40];                 // +0x144..0x1E4 (160 bytes), high confidence (UPGRADED from
                            // MEDIUM, same round/function as `xpermove` above): the immediately
                            // following instructions repeat the identical `onstage`-indexed
                            // pattern at `+0x144`, matching source's "ypermove=cmls->
                            // ypermove[cmls->onstage];" (`AC.CPP:17331`) exactly -- the same
                            // single source line confirms both fields at once. Matches 2011's
                            // declared `fixed ypermove[MAXNEEDSTAGES]` (`acroom.h:3085`) in
                            // position, size, and now behavior.
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
  char direct;                     // +0x1FD, CONFIRMED ABSENT (upgraded from MEDIUM/an
                            // incomplete negative result the previous round, now exhaustive):
                            // ALL THREE of 2011's own write sites for this field were checked
                            // this round and none exist in this build. (1) `move_object` (already
                            // matched, fully read) ends immediately after "objs[objj].moving=
                            // mslot;" with no further write, unlike 2011's "mls[mslot].direct=
                            // ignwal;" (`AC.CPP:16561`) right after it. (2) Character movement:
                            // `MoveCharacterDirect` (already matched) is a thin wrapper that just
                            // calls `walk_character(cc,xx,yy,ignwal=1,autoWalkAnims=0)`; "MoveCharacter"
                            // itself calls the SAME `walk_character` with `ignwal` varying --
                            // `walk_character`'s own body (fully read this round, start to end)
                            // never touches `mls[mslot]+0x1FD`, and its own post-`find_route`
                            // helper (`sub_40EB7B`, called for the "route needs more than one
                            // step" case) was checked too -- zero byte-sized writes anywhere in
                            // its ~420-line body. (3) `NewRoom` (already matched, fully read): its
                            // `inside_script` branch is a genuine simpler predecessor of 2011's
                            // "nasty hack" (`AC.CPP:20021-20030`) -- this build just stores `nrnum`
                            // into the already-confirmed `ExecutingScript.newnum`@+0x04 via the
                            // already-confirmed `curscript` global, with NO
                            // `mls[playerchar->walking].direct=1;`/`StopMoving` logic at all; the
                            // whole hack doesn't exist here. With all three 2011 write sites
                            // checked and none present, `direct` is confirmed absent by the same
                            // "exhaustive multi-site check" standard used elsewhere in this
                            // project (matching 2011's own comment role, "MoveCharDirect was used
                            // or not" -- this build's `MoveObject`/`MoveCharacter` unify the
                            // direct/non-direct distinction entirely through the `ignwal` parameter
                            // passed to `find_route`, with no separate persistent flag needed).
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
  short numloops;                  // +0x00, high confidence (UPGRADED from MEDIUM, found by
                            // cross-referencing a previous round's own `update_stuff` evidence):
                            // the "turning around before walking" branch (see `CharacterInfo.
                            // loop`'s own entry) does "ecx=[this_char+8] /*view*/; imul ecx,8D4h;
                            // edx=views_global; eax=word[edx+ecx]" -- reading `views[view]+0`
                            // with NO added offset, i.e. `numloops` itself -- then compares it
                            // against `turnlooporder[wantloop]`, matching source's
                            // "turnlooporder[wantloop] >= views[chi->view].numLoops" exactly.
                            // Sits with zero slack immediately before the confirmed `numframes[]`
                            // below, matching 2011's declared field order ("short numloops; short
                            // numframes[16];", `acroom.h:2421-2422`) exactly in position and type.
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
                            // the global's own identity as AGS's `objs[]` array. SECOND
                            // independent confirmation (found while chasing the stop_fast_forwarding
                            // lead): `EndSkippingUntilCharStops`/`unload_old_room`-combined
                            // (already matched) zeroes this same field in a
                            // "for(ff=0;ff<croom->numobj;ff++) [dword_4E45C8+ff*20h+18h]=0" loop,
                            // matching 2011's "for (ff=0;ff<croom->numobj;ff++) objs[ff].moving=0;"
                            // (AC.CPP:3597-3598) exactly. (This build's
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
                            // NOWALKBEHINDS`(2). THIRD confirmation of bit 1 (found this round):
                            // `SetObjectIgnoreWalkbehinds` (newly matched, exact match to
                            // `AC.CPP:20911-20919`) clears then conditionally re-sets this exact
                            // bit ("and al,0FDh" / "or dl,2") in direct response to its own
                            // script-facing `clik` parameter -- the clearest, most direct
                            // confirmation yet of `OBJF_NOWALKBEHINDS`'s identity and value.
  char _pad_align[2];               // +0x1E..0x20, compiler alignment padding (not a real field)
                            // -- boxed in with zero slack by the confirmed total stride (0x20)
                            // and the confirmed `flags` field ending at +0x1E.
};

struct AnimationStruct {
  // MAJOR CORRECTION (fresh survey follow-up, after this project's earlier session had drafted
  // this struct under the project-assigned placeholder name `EventBlockCmd` and explicitly
  // recorded "no living OR dead-commented 2011 declaration corresponds to this one" -- that claim
  // was WRONG, just not yet disproven at the time). 2011's source DOES declare this struct, under
  // the name `AnimationStruct` (`Common/acroom.h:218-226`): "struct AnimationStruct { int x, y;
  // int data; int object; int speed; char action; char wait; ... };". The correspondence was
  // found by working the arithmetic backward from `FullAnimation` (`acroom.h:228-232`,
  // "AnimationStruct stage[MAXANIMSTAGES]; int numstages;", MAXANIMSTAGES=10, acroom.h:217) --
  // 10 * sizeof(AnimationStruct) + sizeof(int) lands EXACTLY on this build's already-confirmed
  // `GameAnimation` total size (0xF4/244 bytes) with zero slack IF AnimationStruct itself is
  // 24 bytes (5 ints + 2 chars, naturally padded to a 4-byte boundary -- exactly this struct's
  // own already-confirmed 0x18-byte stride). Checking field-by-field against sub_40C3E0's
  // already-fully-characterized per-record dispatcher (see its own matches.json entry) confirms
  // an exact, unforced semantic correspondence for EVERY field, not just the total size: `x`@+0x00
  // (this struct's `data0`, "target X coordinate"), `y`@+0x04 (`data1`, "target Y coordinate"),
  // `data`@+0x08 (`data2`, "view/loop number" -- 2011's OWN field name is the same generic "data"
  // this project had already independently guessed), `object`@+0x0C (`target`, "the entity
  // selector" -- 2011's OWN field name is literally "object", matching this project's own
  // independently-derived semantic description almost word for word), `speed`@+0x10 (`data3`,
  // "the speed parameter" -- exact name match), `action`@+0x14 (`type`, "the command type byte" --
  // 2011's own field is a 1-byte discriminator here renamed `action`), `wait`@+0x15
  // (`waitUntilDone`, "gates a blocking wait" -- exact semantic and even near-exact name match).
  // Every one of these fields was independently derived from pure disassembly evidence, with zero
  // knowledge of AnimationStruct's existence at the time -- an unusually strong, over-determined
  // confirmation. RENAMED from `EventBlockCmd` to this struct's real 2011 name; the old name is
  // kept in this comment (and left as a documented former name in matches.json/struct-layout-
  // drift.md, per this project's "visible retraction, not silent edit" convention) purely for
  // searchability across older notes. See `FullAnimation` below for the outer array/count struct
  // and `RoomStruct.anims[10]`'s own entry (now retyped to `FullAnimation anims[10]` directly) for
  // a THIRD independent confirmation: 2011 declares `roomstruct.anims` as exactly
  // `FullAnimation anims[MAXANIMS]; short numanims;` (`acroom.h:835-836`), matching this build's
  // already-confirmed field name, capacity (MAXANIMS=10, zero drift), and adjacency exactly.
  //
  // ARCHITECTURAL NOTE (unaffected by this rename): 2011 itself still DECLARES this struct and
  // still reads `numanims`/`numstages`-shaped counts from room files, but no longer actually
  // reads the `anims[]`/`stage[]` PAYLOAD data on load for current room versions -- see
  // `acroom.h:1897-1908`, where the real "fread(&rstruc->anims[0], sizeof(FullAnimation), ...)"
  // is commented out in favor of an `fseek` that just skips past the bytes. This is the exact
  // same "still fully live in this 2002 build, reduced to a dead/skipped legacy declaration by
  // 2011" pattern already seen with `RoomStruct.hscond`/`.objcond`/`.misccond` (EventBlock-based
  // room interaction data) -- not a corrupted or misread version of the 2011 struct, a genuinely
  // still-functioning ancestor subsystem 2011 stopped actually using but never deleted the
  // declaration for. This build's own actual command-list PROCESSING (via `sub_40C3E0`, still
  // unnamed, and `run_animation` -- see their own matches.json entries; `run_animation` itself
  // was matched in a later round via `wait_loop_still_valid`'s own dead-in-2011
  // `if(user_disabled_for==FOR_ANIMATION) run_animation((FullAnimation*)user_disabled_data2,
  // user_disabled_data3);` call site, `AC.CPP:25723-25724` -- the ONLY place this exact name
  // and call shape survive in the 2011 source, commented out but still textually present) has
  // NO LIVING 2011 counterpart; only the underlying DATA LAYOUT (AnimationStruct/FullAnimation)
  // survives to 2011, unused. `run_interaction_commandlist`/`NewInteractionCommand` (`acroom.h:600`,
  // `Engine/AC.CPP:21449`) remains a separate, structurally unrelated, much later replacement
  // system (vtable-based, 5 typed `data[]` slots, ~76+ bytes) covering a similar action set
  // (set/release view, animate, move with/without wall-avoidance, set position) -- not this
  // struct's own direct successor.
  //
  // DECISIVE FOLLOW-UP CONFIRMATION (found in the very next round, while investigating an
  // unrelated `RoomStruct` gap): `sub_4247A0` (RESOLVED, previously unnamed -- see its own
  // matches.json entry), the per-element constructor callback for `FullAnimation.stage[10]`'s own
  // C++ array-of-objects default-construction (called from `sub_424770`/`FullAnimation__
  // FullAnimation` with ElementSize=0x18(24)/Count=0xA(10), independently reconfirming this
  // struct's own stride/capacity a THIRD way), has a body of EXACTLY four literal writes:
  // "[this+0x14]=0; [this+0xC]=0; [this+0x15]=1; [this+0x10]=5;" -- matching 2011's own
  // `AnimationStruct() { action=0; object=0; wait=1; speed=5; }` (`acroom.h:225`) WORD FOR WORD AND
  // VALUE FOR VALUE, with the same four fields the disassembly-only investigation had already
  // (independently) matched to `action`/`object`/`wait`/`speed`. This is about as strong as
  // confirmation gets in this project -- a full constructor-literal match, not just a size or
  // single-field fit -- and definitively puts to rest any remaining doubt about the
  // `EventBlockCmd`->`AnimationStruct` rename.
  int x;                             // +0x00 (was `data0`), high confidence: the target X
                            // coordinate for type 3/4 (move, passed as `tox` to
                            // `move_object`/`walk_character`) and for type 5 (direct position
                            // set, written straight to `.x`). Unused by types 1/2. Matches
                            // 2011's declared `AnimationStruct::x` (`acroom.h:219`) exactly.
  int y;                             // +0x04 (was `data1`), high confidence: the target Y
                            // coordinate for type 3/4/5 (`toy`, or written straight to `.y`),
                            // but REUSED for type 1/2 as a single-bit flag (`and ecx,1`, a
                            // "repeat"-style gate). Matches 2011's declared `AnimationStruct::y`
                            // (`acroom.h:219`) in position; the type-1/2 flag reuse is this
                            // build's own behavior, not separately declared in 2011.
  int data;                          // +0x08 (was `data2`), high confidence: the view number for
                            // type 1 (`SetObjectView`/`SetCharacterView`'s `vii`; `==0` selects
                            // `ReleaseCharacterView` instead for a character target) and the
                            // loop number for type 2's `AnimateObject` (object path). Unused by
                            // types 3/4/5. Matches 2011's declared `AnimationStruct::data`
                            // (`acroom.h:220`) exactly, including the field NAME.
  int object;                        // +0x0C (was `target`), high confidence: the entity
                            // selector, decoded once near the top of sub_40C3E0 and used by
                            // every type -- this build's established object/character
                            // convention (<10 = room object index, ==99 = player character,
                            // >=100 = character index+100, matching the same convention
                            // documented for `AnimateObject`'s obn>=99 branch to
                            // `animate_character`). Matches 2011's declared
                            // `AnimationStruct::object` (`acroom.h:221`) exactly, including the
                            // field NAME.
  int speed;                         // +0x10 (was `data3`), high confidence: the speed parameter
                            // for type 2's `AnimateObject` (object path, `spdd`) and type 3/4's
                            // move speed (`spee`, passed to `move_object`/packed into
                            // `walk_character`'s call); also packed (shifted left 8 bits) into
                            // CharacterInfo+0x3E for type 2's character path. Unused by types
                            // 1/5. Matches 2011's declared `AnimationStruct::speed`
                            // (`acroom.h:222`) exactly, including the field NAME.
  char action;                       // +0x14 (was `type`), high confidence: the command type
                            // byte, fully enumerated 0-5 -- type 0 = explicit error
                            // ("!undefined animation command"); type 1 = SetObjectView (target
                            // is a room object) or SetCharacterView/ReleaseCharacterView (target
                            // is a character -- UNIFIED into one type here via a `data==0`
                            // sentinel meaning "release", where 2011 later SPLIT this into two
                            // separate `NewInteractionCommand` types, `case 27`/`case 28`); type
                            // 2 = AnimateObject (object) or an inline animate-character
                            // equivalent (character, packing a value into CharacterInfo+0x3E);
                            // type 3 = `move_object`/`walk_character` with `ignwal=0` (respect
                            // walkable areas); type 4 = the same, with `ignwal=1` ("move direct",
                            // ignoring walls); type 5 = set the target's `x`/`y` position
                            // directly with no movement at all (writes straight into
                            // `RoomObject.x`/`.y` for an object, or `CharacterInfo+0x14`/`+0x18`
                            // for a character). Any other value hits a SECOND distinct error,
                            // "unknown animation encountered", proving the switch is exhaustive
                            // -- no further types exist in this build. Matches 2011's declared
                            // `AnimationStruct::action` (`acroom.h:223`) in position and role
                            // (a type/action discriminator byte), though 2011's own constructor
                            // (`action=0;`) never assigns it the richer 1-5 meaning this build's
                            // command-list interpreter (`sub_40C3E0`, still unnamed) actively
                            // uses -- consistent with the "declared but functionally dead by
                            // 2011" pattern documented at the struct level above.
  char wait;                         // +0x15 (was `waitUntilDone`), high confidence: gates a
                            // blocking `do_main_cycle` call after types 2 (character path), 3,
                            // and 4 -- "wait until this animation/movement finishes before
                            // continuing". Matches 2011's declared `AnimationStruct::wait`
                            // (`acroom.h:224`) exactly, including the field NAME -- 2011's own
                            // constructor even defaults it to `wait=1`, consistent with this
                            // being a "should the engine block" flag in both builds.
  char _pad_align[2];               // +0x16..0x18, compiler alignment padding (not a real field)
                            // -- boxed in with zero slack now that every other byte through
                            // +0x15 is accounted for across all 6 command types, and matching
                            // the same natural 4-byte-alignment padding 2011's own declaration
                            // (5 ints + 2 chars = 22 bytes, rounded to 24) would produce.
};

struct FullAnimation {
  // MAJOR CORRECTION (fresh survey follow-up, same round as `AnimationStruct` above): this
  // struct was originally drafted under the project-assigned placeholder name `GameAnimation`,
  // documented as "a genuine, previously entirely unknown game resource... entirely absent from
  // the 2011 reference source". That conclusion is WRONG in one specific way: the DATA FORMAT
  // (this struct) is not absent from 2011 at all -- it's declared verbatim as `FullAnimation`
  // (`Common/acroom.h:228-232`: "struct FullAnimation { AnimationStruct stage[MAXANIMSTAGES]; int
  // numstages; ... };", MAXANIMSTAGES=10 at `acroom.h:217`) and is STILL `RoomStruct`'s own
  // `anims[MAXANIMS=10]` field type in the CURRENT 2011 source (`acroom.h:835`) -- see
  // `RoomStruct.anims`'s own entry, now retyped from a raw byte blob to `FullAnimation anims[10]`
  // directly. What genuinely IS new/undocumented in 2011 is this build's own PROCESSING of that
  // data as a standalone, room-independent, globally-numbered resource table (`unk_52024C[10]`,
  // triggerable from any EventBlock's `respond[i]==4` "Run Animation" via `EventBlock.data[i]` as
  // a 0-9 index) -- matching the OLD AGS Editor's "Animations" resource pane, a distinct
  // project-tree entry type in ancient AGS versions that predates the modern Views-only approach
  // and is gone entirely from both 2011's UI and its actual room-loading code path (which now
  // only reads `numanims` and skips the payload -- see `AnimationStruct`'s struct-level comment).
  // So: the FORMAT survives to 2011 (dead but declared); this build's specific USE of that format
  // as its own global resource table does not. RENAMED from `GameAnimation` to this struct's real
  // 2011 name; old name kept in this comment for searchability, per this project's "visible
  // retraction" convention.
  //
  // Confirmed via `run_event_block` (already matched): its `respond[i]==4` branch bounds-checks
  // `EventBlock.data[i]` against `0Ah`(10) -- erroring "!run_animate: undefined animation was
  // r[un]" if out of range -- giving MAX_ANIMATIONS=10 for this build. It then checks
  // `dword_52033C[data[i]*0xF4]` for nonzero, erroring "!Run_animate: empty animation was run"
  // otherwise, before finally calling run_animation(&unk_52024C[data[i]*0xF4], 0) -- `unk_52024C`
  // being the actual table of this struct's instances, `run_animation` being the already-
  // characterized `AnimationStruct` list iterator (see its own entry).
  //
  // RESOLVED (a follow-up round, after this struct was first drafted): `dword_52033C` is NOT a
  // separate parallel table at all -- its address (0x52033C) is EXACTLY `unk_52024C`'s address
  // (0x52024C) plus `0xF0`, i.e. `&unk_52024C[0].numstages`. IDA simply assigned it a distinct
  // symbol because the compiler generated that one access as a literal computed address rather
  // than as `unk_52024C+member_offset`, so IDA's data-xref analysis didn't recognize the overlap.
  // The "empty animation" check is therefore just `FullAnimation[data[i]].numstages != 0` --
  // the exact same field `run_animation`'s own loop bound reads, checked once early as an
  // error-message-friendly short-circuit before the (otherwise silently-no-op) iteration. This
  // ALSO gives `numstages`@+0xF0 below a second, fully independent confirmation.
  //
  // Total size 0xF4(244 bytes) is high confidence: independently confirmed by FOUR things
  // landing on it simultaneously -- the `dword_52033C`/`unk_52024C` address delta itself
  // (`0xF0`, i.e. exactly `numstages`'s own offset), run_animation's own confirmed "list[+0xF0] =
  // numstages" access applied to `unk_52024C[slot]` (`stage[10]` (10*0x18=0xF0) plus a trailing
  // `numstages` int lands EXACTLY on the externally-confirmed 0xF4 stride with zero slack), 2011's
  // own declared arithmetic (10 * sizeof(AnimationStruct)=0x18 + sizeof(int) = 0xF4) reached
  // completely independently, from the reference source rather than the disassembly, AND (found a
  // round later) `FullAnimation__FullAnimation`'s own C++ constructor body: a `stage[]`
  // array-of-objects default-construction call with ElementSize=0x18(24)/Count=0xA(10) followed
  // immediately by "[this+0xF0]=0" -- matching 2011's own `FullAnimation() { numstages=0; }`
  // (`acroom.h:231`) precisely, and independently reconfirming `numstages`@+0xF0 a THIRD way. See
  // `AnimationStruct`'s own struct-level comment for the matching decisive confirmation of ITS
  // fields via the same constructor-chasing round (`sub_4247A0`/`AnimationStruct__
  // AnimationStruct`, `stage[]`'s own per-element constructor).
  AnimationStruct stage[10];         // +0x000..0xF0 (240 bytes), high confidence: see
                            // struct-level comment -- MAX_ANIMATIONS=10 slots confirmed via
                            // run_event_block's own literal `0Ah` bounds check (matching 2011's
                            // own `MAXANIMSTAGES=10`, `acroom.h:217`, with zero drift), and this
                            // array's own per-element stride (0x18) already independently
                            // confirmed via `AnimationStruct`. Matches 2011's declared
                            // `AnimationStruct stage[MAXANIMSTAGES]` (`acroom.h:229`) exactly,
                            // including the field NAME (was `command` under the old placeholder
                            // name).
  int numstages;                     // +0xF0 (was `numCommands`), high confidence: confirmed via
                            // run_animation (already characterized): the loop bound for iterating
                            // `stage[]`. INDEPENDENTLY reconfirmed via `run_event_block`'s
                            // "empty animation" check reading this exact same field through the
                            // address it knows as `dword_52033C` -- see the struct-level
                            // RESOLVED note above. Matches 2011's declared `int numstages`
                            // (`acroom.h:230`) exactly, including the field NAME.
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
  // `intrRoom` (already proven entirely absent from this build, see `AnimationStruct`/
  // `FullAnimation` -- formerly `EventBlockCmd`/`GameAnimation`, renamed after their real 2011
  // identities were found) turns out to hold this build's OWN EventBlock-based per-room
  // interaction data instead -- `hscond[20]`/`objcond[10]`/`misccond` below, matching 2011's OWN
  // dead-commented-out declaration for exactly this (`Common/acruntim.h:105-107`) almost verbatim.
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
  RoomObject obj[10];                // +0x08..0x148 (320 bytes), high confidence (UPGRADED from
                            // MEDIUM by connecting evidence already on record elsewhere -- see
                            // `RoomObject.transparent`'s own entry above, which already cites this
                            // SAME `load_new_room` loop for a single field without the connection
                            // being propagated back here): the room's first-time "beenhere==0"
                            // initialization loop -- "for(chaa=0; chaa<[croom+4]/*numobj*/; chaa++)"
                            // -- writes NINE separate `RoomObject` fields per iteration, every one
                            // at `[dword_523128/*croom*/ + chaa*0x20 + FIELD_OFFSET]`: `x`@+0x00
                            // (from `thisroom.sprs[chaa].x`), `y`@+0x04 (from `thisroom.
                            // sprs[chaa].y`, height-adjusted), `num`@+0x0C (from `thisroom.
                            // sprs[chaa].sprnum`), `transparent`@+0x08 (already its own entry's
                            // evidence), `view`@+0x10=-1, `loop`@+0x12=0, `frame`@+0x14=0,
                            // `wait`@+0x16=0, `moving`@+0x18=-1, `baseline`@+0x0E=-1 (then
                            // conditionally overwritten from `RoomStruct.objbaseline[chaa]`, the
                            // already-confirmed global, if `>=0`) -- matching 2011's own
                            // `croom->obj[cc].FIELD=...` initialization block (`Engine/AC.CPP:
                            // 4282-4298`) field for field, in the same order, with the SAME
                            // defaults. This is no longer an arithmetic fit -- it's direct,
                            // exhaustive, per-field behavioral confirmation that `obj[]` genuinely
                            // starts at `+0x08` within `RoomStatus`. DRIFT: 10 objects/room here
                            // vs. 2011's declared `MAX_INIT_SPR=40` (`acroom.h:59`) -- a 4x
                            // reduction, consistent with this project's "smaller fixed capacity"
                            // pattern, and matching `RoomObject`'s own independently-confirmed
                            // array capacity elsewhere in this project with zero further drift.
  short flagstates[15];              // +0x148..0x166 (30 bytes), high confidence (UPGRADED from
                            // MEDIUM, found in a later round): `load_new_room` (already matched)
                            // -- immediately after the `obj[]` initialization loop just confirmed
                            // above -- does "for(chaa=0; chaa<0Fh(15); chaa++)
                            // [dword_523128/*croom*/+chaa*2+0x148]=0" -- a direct, literal,
                            // zero-ambiguity confirmation of BOTH this field's exact position
                            // AND its exact capacity in one shot, matching 2011's "for
                            // (cc=0;cc<MAX_FLAGS;cc++) croom->flagstates[cc]=0;"
                            // (`Engine/AC.CPP:4308`) exactly, with `MAX_FLAGS=15`
                            // (`acroom.h:801`) confirmed with ZERO drift via the literal loop
                            // bound itself, not just an arithmetic remainder. The SAME code
                            // region immediately afterward does three more `rep movsd` block
                            // copies of 148-byte(`0x94`) `EventBlock` records from `RoomStruct`-
                            // side source data -- one record into `misccond`@+0x12C8, then a
                            // 20-element loop into `hscond[20]`@+0x170, then a 10-element loop
                            // into `objcond[10]`@+0xD00 -- a further, independent reconfirmation
                            // of all three via the exact same evidence pass.
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
                            // documented capacity rather than a smaller ad-hoc reduction. DATED
                            // AND RECONCILED (via `ags-archives/`, see `reversing/notes/
                            // ags-archives-cross-reference.md`): `ags230/docs/CHANGES.TXT`'s
                            // "VERSION 2.3, January 2002" entry says "Upped limit to 19 hotspots
                            // (sorry, quick fix, more later)" -- at first glance a mismatch (19 vs.
                            // this build's confirmed 20-slot array), but it reconciles exactly: 19
                            // USABLE hotspots (matching this same field's own confirmed `hsnum`
                            // bounds check, 1 through 19 inclusive) plus hotspot 0 (reserved for
                            // "no hotspot"/background, never individually enabled/disabled) is
                            // precisely a 20-element array. No later CHANGES.TXT entry through
                            // Rob Blanc 1's own ~2.4b era raises this further, consistent with
                            // 2011's own comment that the NEXT increase (to 30) waited until v2.62.
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
  int gscript_timer;          // +0x0C, HIGH confidence (UPGRADED from medium-high): confirmed
                            // via the graph-script command interpreter (`sub_41CDC3`, newly
                            // characterized this round -- see `run_graph_script`'s own matches.json
                            // entry): its SET_TIMER opcode (case 23) does
                            // "play_gscript_timer=record[+5]; if(record[+5]==0)
                            // play_gscript_timer=-1;", and its IF_TIMER_EXPIRED opcode (case 24)
                            // checks "play_gscript_timer==0" before resetting it to -1 and
                            // recursing into a nested command list -- a direct, individually
                            // confirmed instruction match this field previously lacked.
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
  int usedinv;                 // +0xE0, HIGH confidence (UPGRADED, resolving the prior session's
                            // own "?"-flagged uncertainty): confirmed via the graph-script
                            // command interpreter's (`sub_41CDC3`, newly characterized this
                            // round) IF_USED_INVENTORY_ITEM opcode (case 26): "if(play_usedinv==
                            // record[+5]) recurse(...)" -- matching 2011's own "case 20: // If
                            // Inventory Item was used -- if(play.usedinv==IPARAM1){...}"
                            // (`Engine/AC.CPP:21557-21558`) almost verbatim, down to the same
                            // "if used-inv equals this item, run a nested command list" shape.
                            // Distinct from `used_inv_on`@+0x128 (a different, already-confirmed
                            // field -- "which object/hotspot the item was used ON", not "which
                            // item was used").
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
  int text_speed;               // +0xF4, high confidence (UPGRADED from tentative): an
                            // unmatched helper (sub_4136AF) computes "(strlen(Str)/
                            // dword_4EEB0C + 1) * frames_per_second", matching the role of
                            // 2011's text-display-duration calculation (AC.CPP:12688-12692,
                            // this build's simpler predecessor lacking later-added modifiers).
                            // Independently confirmed via game-startup init setting it to the
                            // literal value 15, matching 2011's "play.text_speed=15;"
                            // (AC.CPP:26289) exactly.
  int sierra_inv_color;         // +0xF8, high confidence (UPGRADED from tentative):
                            // `__actual_invscreen` (already matched) does "push dword_4EEB10;
                            // call sub_40187F(wsetcolor-equivalent);" immediately before
                            // drawing the inventory window background, matching 2011's
                            // "wsetcolor(play.sierra_inv_color); wbar(...);"
                            // (AC.CPP:23916-23917) exactly.
  int talkanim_speed;           // +0xFC, high confidence (UPGRADED from tentative):
                            // `_displayspeech` (already matched) packs this into a
                            // CharacterInfo field via the classic AGS packed-value idiom when
                            // starting a talk animation, and it's independently confirmed set
                            // to the literal value 5 during init, matching 2011's
                            // "play.talkanim_speed=5;" (AC.CPP:26279) exactly. NOTE: 2011's own
                            // source only ever assigns this field once (at init) and never
                            // reads it again -- this build actively USES it, another case
                            // (like inv_numorder) of a field 2011 kept declared but stopped
                            // actively using.
  int inv_item_wid;            // +0x100, high confidence: confirmed via sub_40D80C as the
                            // column-width divisor -- "mover = mouseX/(inv_item_wid*mult_x)"
                            // matches source's "mover = xoffs/itemWidth" (`GUIInv::itemWidth`,
                            // set by `SetInvDimensions`, AC.CPP:24106) exactly. Matches 2011's
                            // exact declared field name and adjacent pairing with `inv_item_hit`.
  int inv_item_hit;            // +0x104, high confidence: confirmed via sub_40D80C as the
                            // row-height divisor, same match shape as `inv_item_wid` above.
  int speech_text_shadow;       // +0x108, high confidence (UPGRADED from tentative): an
                            // unmatched helper (sub_413635, called from `GUITextBox::Draw`
                            // among others) reads dword_4EEB20 immediately before a
                            // wtextcolor-equivalent call (sub_401F62), matching 2011's
                            // "wtextcolor(play.speech_text_shadow);" (AC.CPP:12616/12621)
                            // exactly in role. Independently confirmed via `main`'s init
                            // block setting dword_4EEB20=0x10(16), matching 2011's
                            // "play.speech_text_shadow = 16;" (AC.CPP:26338) exactly.
  int swap_portrait_side;      // +0x10C, HIGH confidence (UPGRADED from tentative several
                            // rounds later): `_displayspeech` (already matched) does "if
                            // (dword_4EF2B8 != xx) { if (dword_4EEB24==1) dword_4EEB24=2; else
                            // if (dword_4EEB24==2) dword_4EEB24=1; ... }" matching 2011's "if
                            // (play.swap_portrait_lastchar != aschar) { ... if
                            // (play.swap_portrait_side==2) play.swap_portrait_side=1; else
                            // play.swap_portrait_side=2; ... }" (AC.CPP:13697-13721) exactly --
                            // the original positional guess for this field is confirmed
                            // correct.
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
  int follow_change_room_timer; // +0x114, high confidence (UPGRADED from unidentified after
                            // FIVE rounds): `main`'s inlined game-settings-init block
                            // (already matched) sets dword_4EEB2C=0x96(150), matching 2011's
                            // "play.follow_change_room_timer = 150;" (AC.CPP:26394) exactly.
  int totalscore;               // +0x118, high confidence: `replace_macro_tokens` (already
                            // matched) reads this for BOTH its "totalscore" and "scoretext"
                            // macro branches, matching 2011's `#define MAXSCORE
                            // play.totalscore` (`acruntim.h:809`) exactly -- 2011's own source
                            // uses the macro rather than the field name directly at both call
                            // sites (`AC.CPP:7134`/`7136`), but the macro's definition makes
                            // the identification unambiguous. Reconfirmed via `main`'s init
                            // block: "dword_51B84C(game.totalscore) -> dword_4EEB30" matches
                            // "play.totalscore = game.totalscore;" (AC.CPP:26348) exactly,
                            // also identifying dword_51B84C as the bonus global game.totalscore.
  int skip_display;             // +0x11C, high confidence (UPGRADED from medium): `main`'s init
                            // block sets dword_4EEB34=3, matching 2011's "play.skip_display =
                            // 3;" (AC.CPP:26344) exactly -- on top of the pre-existing
                            // message-box-wait-loop role match from `_display_main`.
  int no_multiloop_repeat;      // +0x120, high confidence (UPGRADED from unidentified after
                            // FIVE rounds): `main`'s init block sets dword_4EEB38=0 in the
                            // same sequential position 2011 declares/initializes
                            // no_multiloop_repeat (immediately after skip_display), matching
                            // "play.no_multiloop_repeat = 0;" (AC.CPP:26345).
  int roomscript_finished;      // +0x124, high confidence: `post_script_cleanup` (already
                            // matched)'s `runnext[0]=='$'` branch does
                            // "run_text_script_iparam(roominst,...); dword_4EEB3C=1;" matching
                            // 2011's "run_text_script_iparam(roominst,&runnext[1],...);
                            // play.roomscript_finished = 1;" (AC.CPP:3179-3181) exactly.
                            // Reconfirmed via `main`'s init block setting it to 0.
  int used_inv_on;              // +0x128, high confidence: `check_controls` (already matched)'s
                            // GOBJ_INVENTORY click branch does "iit=sub_40D80C(); if (iit>=0)
                            // dword_4EEB40=iit;" matching 2011's "int
                            // iit=offset_over_inv(...); if (iit>=0) { ...; play.used_inv_on =
                            // iit; }" (AC.CPP:5707-5710) exactly -- also a further independent
                            // confirmation that sub_40D80C is this build's offset_over_inv
                            // equivalent.
  int no_textbg_when_voice;     // +0x12C, high confidence (UPGRADED from unidentified after
                            // FIVE rounds -- an earlier round's `skip_display` guess for this
                            // specific field is RETRACTED, see `skip_display`@+0x11C above for
                            // the correct field): `main`'s init block sets dword_4EEB44=0 in
                            // the same sequential position 2011 declares/initializes
                            // no_textbg_when_voice (immediately after roomscript_finished),
                            // matching "play.no_textbg_when_voice = 0;" (AC.CPP:26350). SECOND,
                            // independent confirmation (GameState remaining-fields round,
                            // chasing the music_master_volume cluster): `play_speech` (this
                            // round's new match, `sub_4141B8`)'s tail does "if
                            // (byte_513340==2 && dword_4EEB44>0) { byte_513340=1;
                            // dword_4EEB44=2; }" matching 2011's "if
                            // ((game.options[OPT_SPEECHTYPE]==2) &&
                            // (play.no_textbg_when_voice>0)) { game.options[OPT_SPEECHTYPE]=1;
                            // play.no_textbg_when_voice=2; }" (AC.CPP:13428-13431) exactly --
                            // also identifies `byte_513340` as `game.options[OPT_SPEECHTYPE]`.
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
                            // 2011's declared MAXGSVALUES=500. DATED (via `ags-archives/`, see
                            // `reversing/notes/ags-archives-cross-reference.md`): `ags222/docs/
                            // CHANGES.TXT`'s "VERSION 2.22, December 2001" entry says "Upped
                            // GlobalInts to 300" -- an exact match, meaning this exact 300-value
                            // capacity was introduced December 2001 and stayed unchanged through
                            // Rob Blanc 1's own ~2.4b era before later growing to 2011's 500.
  // The +0x60C..+0x80C span (512 bytes) is now FULLY BYTE-ACCOUNTED FOR, and unlike when this
  // comment was first written, its first 8 bytes (cur_music_number/music_repeat, immediately
  // below) are now real confirmed fields, not a pad -- the rest is broken into precisely-sized
  // pieces below rather than one undifferentiated pad. IMPORTANT CAUTION established here and
  // reinforced since: falling
  // inside SaveGameSlot's proven 2404-byte fwrite span (see this struct's header comment) is NOT
  // by itself sufficient evidence of GameState membership -- role-based confirmation is still
  // required, as it was for every field already confirmed in this struct. This span is known to
  // contain at least one WHOLE UNRELATED STRUCT (CharacterExtras, below), not just uncharted
  // GameState fields or scratch memory -- the fwrite's literal size constant sweeps in more than
  // the true GameState struct, capturing adjacent-but-distinct AC.CPP file-scope globals that
  // the linker happened to place contiguously after `play`.
  int cur_music_number;         // +0x60C, high confidence (RESOLVED, closing a pad open since
                            // this struct's earliest rounds): `GetCurrentMusic` (already matched)
                            // does "return dword_4EF024;" matching 2011's
                            // "return play.cur_music_number;" (AC.CPP:17750) exactly. Cross-
                            // confirmed across 5 more functions: `PlayMusic` (already matched)
                            // reads it for an "already playing this track" early-out then writes
                            // the new music number, matching AC.CPP:17896-17917 exactly;
                            // `scr_StopMusic` (already matched) writes -1, matching
                            // "play.cur_music_number=-1;" (AC.CPP:17583); `restore_game_data`
                            // (already matched) writes the literal 0x7D0(2000), matching source's
                            // own "play.cur_music_number=2000; // make sure it gets played"
                            // (AC.CPP:23632) -- an unusually specific literal that could not
                            // plausibly be a coincidence; and `main` (already matched) inits it to
                            // -1, matching AC.CPP:26327.
  int music_repeat;             // +0x610, high confidence: `SetMusicRepeat` (already matched)'s
                            // ENTIRE one-line body -- "dword_4EF028 = loopflag;" -- matches 2011's
                            // ENTIRE function body verbatim, "void SetMusicRepeat(int loopflag) {
                            // play.music_repeat=loopflag; }" (AC.CPP:17753-17755). Sits with ZERO
                            // gap immediately after `cur_music_number` above, matching 2011's exact
                            // declared adjacency "int cur_music_number,music_repeat;"
                            // (acruntim.h:553) with zero drift -- unlike most of this struct's
                            // other confirmed pairs, this one is NOT drift-shifted relative to
                            // 2011's declared order at all, it just isn't contiguous with the
                            // REST of GameState's music fields (`music_master_volume` sits far
                            // away at +0x808 -- see its own entry and the correction noted there).
                            // `PlayMusic` (already matched) reads this as the repeat flag passed
                            // down to its MP3-stream-creation helper.
  char _pad_invorder_maybe[0xC8]; // +0x614..0x6DC (200 bytes) -- NOT asserted as a struct
                            // member; kept as a neutral pad (not a typed `play_invorder[100]`
                            // declaration) since GameState membership here is genuinely
                            // unresolved, consistent with this project's convention of only
                            // giving real field declarations to confirmed members. This is
                            // `play_invorder`, this build's inventory-order array (role
                            // confirmed via `update_invorder`'s exact algorithmic match;
                            // capacity confirmed via a clean, zero-interruption 200-byte span
                            // matching MAX_INV=100 with zero drift) -- but whether it's a
                            // genuine GameState member or, like CharacterExtras immediately
                            // after it, a coincidentally-adjacent separate global remains
                            // UNRESOLVED. Neither neighbor (the unidentified pair above, or
                            // CharacterExtras below) is itself a confirmed GameState field, so
                            // there's no positional evidence either way -- unlike
                            // bad_parsed_word/screen_tint, which each closed against an
                            // independently-confirmed neighbor.
  // CharacterExtras.width/height/zoom (this build's version) -- +0x6DC..0x808 (300 bytes),
  // CONFIRMED NOT GameState -- see the dedicated CharacterExtras documentation block after this
  // struct's closing brace for the full field-by-field writeup and evidence.
  //
  // ADDITIONAL EVIDENCE toward `play_invorder`'s membership question (found this round, via
  // `add_inventory` -- newly given full field evidence, see its own matches.json entry): its
  // body treats `play_invorder[]` and `play_inv_numorder`(=`inv_numorder`@+0xEC, an ALREADY-
  // CONFIRMED GameState member) as one atomic, always-synchronized pair --
  // "play_invorder[play_inv_numorder]=inum; play_inv_numorder++;" -- with `inv_numorder`
  // exclusively incrementing in lockstep with new `play_invorder[]` entries. 2011's own
  // `add_inventory` mirrors this exactly in spirit: "play.obsolete_inv_numorder =
  // charextra[...].invorder_count;" (`AC.CPP:16035`) -- 2011's OWN successor to this same
  // "count" field is likewise always kept synchronized with its own (by-then per-character)
  // order array. This is a DIFFERENT kind of evidence than the positional-adjacency approach
  // that closed `bad_parsed_word`/`screen_tint` -- it's behavioral coupling with a confirmed
  // member, not physical proximity -- and it doesn't by itself PROVE struct membership (this
  // project's own standing limitation still applies: a standalone global and a struct member
  // compile to identical code, so no purely-static technique can fully distinguish them here).
  // But it meaningfully strengthens the case that `play_invorder[]` is conceptually part of the
  // same inventory-order STATE as `inv_numorder`, even if its physical placement can't be
  // proven contiguous. Left as still-unresolved per this project's convention, but no longer
  // "no evidence either way" -- there is now a real, if inconclusive, behavioral argument FOR
  // membership.
  char _pad_characterextras[0x12C];
  int music_master_volume;      // +0x808, high confidence (RESOLVED, correcting the previous
                            // round's "plausibly lipsync/close-mouth-timing related" guess --
                            // WRONG, the real answer is music volume): `sub_418E82` (this round's
                            // new match, `update_music_volume` fused with `calculate_max_volume`)
                            // computes "newvol = thisroom.options[ST_VOLUME]*30 +
                            // dword_4EF220" then clamps to [0,255], matching 2011's
                            // "newvol=play.music_master_volume + thisroom.options[ST_VOLUME]*30;
                            // if(newvol>255)newvol=255; if(newvol<0)newvol=0;" (AC.CPP:12318-
                            // 12320) exactly. `play_speech` (this round's new match, `sub_4141B8`)
                            // decrements this field by a hardcoded 60 right before calling
                            // `sub_418E82`, matching 2011's "play.music_master_volume -=
                            // play.speech_music_drop; ...; update_music_volume();"
                            // (AC.CPP:13417-13420) -- this build hardcodes the ducking amount
                            // instead of reading a configurable `speech_music_drop` field
                            // (predates that feature). ZERO-SLACK POSITIONAL CONFIRMATION: this
                            // field ends exactly 4 bytes before the already-confirmed
                            // `walkable_areas_on`@+0x80C, with no gap -- proving 2011's declared
                            // `digital_master_volume` (sitting between `music_master_volume` and
                            // `walkable_areas_on` in 2011's own order, `acruntim.h:554-556`) is
                            // CONFIRMED ABSENT here. CORRECTION (found two rounds later while
                            // closing the +0x60C..+0x614 pad): this comment previously ALSO
                            // claimed `cur_music_number`/`music_repeat` (2011's declared PRECEDING
                            // pair, `acruntim.h:553`) were confirmed absent by this same zero-gap
                            // argument -- WRONG. That argument only bears on what comes AFTER
                            // `music_master_volume`; it says nothing about fields that would
                            // precede it. Both `cur_music_number` and `music_repeat` turn out to
                            // exist after all, just as a standalone pair at a completely different
                            // address (`+0x60C`, see their own field entries above) -- not embedded
                            // contiguously with the rest of GameState's music fields here. See
                            // struct-layout-drift.md for the full correction writeup.
  char walkable_areas_on[16]; // +0x80C..0x81C, high confidence: `EndSkippingUntilCharStops`/
                            // `unload_old_room`-combined (sub_40AAE3, already matched) does
                            // "memset(&byte_4EF224, 1, 0x10);" matching 2011's
                            // "memset(&play.walkable_areas_on[0],1,MAX_WALK_AREAS+1);"
                            // (AC.CPP:3623) exactly -- MAX_WALK_AREAS=15 (acroom.h:250), so
                            // MAX_WALK_AREAS+1=16=0x10 with zero drift.
  short screen_flipped;         // +0x81C, high confidence (UPGRADED from tentative): sits
                            // exactly in the 2-byte gap between walkable_areas_on's confirmed
                            // end and offsets_locked, matching 2011's declared adjacency
                            // "char walkable_areas_on[...]; short screen_flipped; short
                            // offsets_locked;" (acruntim.h:556-558) with zero slack. Directly
                            // confirmed via `main`'s init block: word_4EF234=0 matches 2011's
                            // "play.screen_flipped=0;" (AC.CPP:26331) exactly.
  short offsets_locked;    // +0x81E, high confidence: originally found via sub_40AAE3 zeroing
                            // it immediately after bg_frame_locked (matching source's exact
                            // adjacent assignment order), and its GameState membership -- left
                            // as an open question two rounds ago after the CharacterExtras
                            // correction -- is now REINFORCED by a second, independent,
                            // positional confirmation: it lands exactly 2 bytes after the newly
                            // and separately confirmed walkable_areas_on, matching 2011's own
                            // declared field order with zero slack.
  char _pad_unknown5[0x08];    // +0x820..0x828 (8 bytes, 2 dwords) -- RESOLVED (GameState's last
                            // open pad, closed this round): this is exactly where 2011's `int
                            // entered_at_x,entered_at_y,entered_edge;` (`acruntim.h:559`) would
                            // place `entered_at_x`/`entered_at_y`, both already independently
                            // CONFIRMED ABSENT (see `entered_edge`'s own comment immediately
                            // below -- `load_new_room`'s second edge-computation block writes the
                            // equivalent 2011 assignment into the shared `tox`/`toy` scratch
                            // globals instead of dedicated persistent fields). An exhaustive
                            // search of the ENTIRE disassembly for every address in this 8-byte
                            // range (0x4EF238-0x4EF23F) turns up ZERO xrefs anywhere -- stronger
                            // than a role-based absence finding, this is direct proof no code in
                            // the whole binary ever touches this memory at all. Kept as a neutral
                            // pad rather than typed fields, consistent with this project's
                            // "only confirmed members get real declarations" convention -- there
                            // is nothing here to declare, just 8 bytes of genuine compiler
                            // padding/unused space between `offsets_locked` and `entered_edge`.
                            // This closes out GameState's field-level investigation entirely --
                            // every byte from +0x00 through +0x964 is now either a confirmed
                            // field or an explicitly-explained, evidenced pad.
  int entered_edge;             // +0x828, high confidence: `load_new_room` (already matched)
                            // sets this to -1 (default) then 0/1/2/3 by descending threshold
                            // comparisons against a bonus-identified global (`new_room_pos`),
                            // matching 2011's "play.entered_edge = -1; ... if
                            // (new_room_pos>=4000) play.entered_edge=3; ... >=1000:
                            // entered_edge=0;" (AC.CPP:4453-4499) exactly, same thresholds,
                            // same descending order. A second edge-computation block in the
                            // same function CONFIRMS GameState.entered_at_x/entered_at_y are
                            // ABSENT from this build -- the equivalent source assignment
                            // (AC.CPP:4539-4540) writes into the shared `tox`/`toy` scratch
                            // globals here instead of dedicated persistent fields.
  int want_speech;              // +0x82C, high confidence: pre-existing IDA name
                            // (`play_want_speech`), XREF'd from `SetVoiceMode` (already
                            // correctly named, mechanical match) at exactly the location
                            // matching 2011's "if (play.want_speech<0)
                            // play.want_speech=(-newmod)-1; else play.want_speech=newmod;"
                            // (AC.CPP:13500-13503).
  int cant_skip_speech;         // +0x830, medium-high confidence (UPGRADED from tentative):
                            // positionally exactly where 2011 declares `cant_skip_speech`
                            // (immediately after `want_speech`); `check_controls`'s own check
                            // there ("0 < dword_4EF248 < 3") doesn't cleanly read as a simple
                            // boolean on its own, but `main`'s init block sets it via
                            // "movsx ecx, byte_51333D; dword_4EF248=ecx" -- a COMPUTED value
                            // from a game-options byte, matching the SHAPE of 2011's
                            // "play.cant_skip_speech = user_to_internal_skip_speech(game.
                            // options[OPT_NOSKIPTEXT]);" (AC.CPP:26333) rather than a fixed
                            // literal, reinforcing the identification without fully closing
                            // it (this build's version doesn't visibly call a conversion
                            // function matching user_to_internal_skip_speech's name).
  int stop_dialog_at_end;       // +0x834, medium-high confidence: pre-existing IDA name
                            // (`play_stop_dialog_at_end`), XREF'd from `RunDialog`/`NewRoom`
                            // (both already correctly named) in a role plausibly matching
                            // 2011's dialog-stop-request flag -- but its POSITION here does
                            // NOT match 2011's declared order (2011 places
                            // stop_dialog_at_end much earlier, adjacent to reserved[10] near
                            // the game.-exposed section boundary, acruntim.h:536, not next to
                            // want_speech/entered_edge at acruntim.h:560-561) -- a genuine,
                            // not-yet-explained architectural difference, flagged rather than
                            // silently assumed consistent.
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
  int normal_font;              // +0x894, high confidence: `SetNormalFont` (already matched,
                            // mechanical) does "if (fontnum<0 || fontnum>=dword_51D2EC)
                            // quit(...); dword_4EF2AC=fontnum;" matching 2011's "if
                            // ((fontnum<0) || (fontnum>=game.numfonts))
                            // quit(\"!SetNormalFont: invalid font number.\"); play.normal_font
                            // = fontnum;" (AC.CPP:13468-13472) exactly, exact error string
                            // included, dword_51D2EC=game.numfonts already confirmed. Sits
                            // with ZERO gap immediately after speech_volume, matching 2011's
                            // exact declared adjacency "sound_volume,speech_volume;
                            // normal_font, speech_font;" with zero drift.
  int speech_font;               // +0x898, high confidence (UPGRADED from medium-high): the
                            // pre-existing IDA name `fontid` turned out to be genuine, not
                            // another mislabeling artifact like `ifnum`/`play_want_music` --
                            // `main`'s init block sets fontid=1, matching 2011's "play.
                            // speech_font = 1;" (AC.CPP:26337) exactly, resolving the standing
                            // caution from several rounds ago.
  char key_skip_wait;          // +0x89C, high confidence: pre-existing IDA name
                            // (`play_key_skip_wait`), behaviorally confirmed via
                            // `check_controls` (already matched): "if (play_wait_counter>0 &&
                            // play_key_skip_wait>1) play_wait_counter=0xFFFF;" matches 2011's
                            // "else if ((play.wait_counter > 0) && (play.key_skip_wait > 1))"
                            // (AC.CPP:5742) exactly.
  char _pad_align7[0x03];      // +0x89D..0x8A0, compiler alignment padding (not a real field)
                            // -- confirmed via IDA's own "align 4" directive at this exact
                            // point in the raw .data listing.
  int swap_portrait_lastchar;   // +0x8A0, high confidence: `_displayspeech` (already matched)
                            // does "if (dword_4EF2B8 != xx) { ...toggle swap_portrait_side...;
                            // dword_4EF2B8=xx; }" matching 2011's "if
                            // (play.swap_portrait_lastchar != aschar) { ...toggle
                            // play.swap_portrait_side...; play.swap_portrait_lastchar=ce; }"
                            // (AC.CPP:13697-13721) exactly.
  int seperate_music_lib;       // +0x8A4, high confidence: `IsMusicVoxAvailable` (already
                            // matched, mechanical) does "return play_want_music;" matching
                            // 2011's "return play.seperate_music_lib;" (AC.CPP:13512-13514)
                            // exactly. CORRECTS the pre-existing IDA name `play_want_music`
                            // (misleading -- 2011 has no "want_music" field at all) -- the
                            // third pre-existing custom global name in this project found to
                            // be a mislabeling artifact, after `ifnum`/speech_textwindow_gui.
  int in_conversation;          // +0x8A8, high confidence: `do_conversation` (already matched)
                            // does "dword_4EF2C0++;" right at its start, matching 2011's
                            // "play.in_conversation++;" (RunDialog, AC.CPP:21955) -- same
                            // role, same position at the top of the dialog-running routine.
  int screen_tint;              // +0x8AC, high confidence: pre-existing IDA name
                            // (`play_scren_tint`, typo-preserved), role confirmed via
                            // `TintScreen` (already matched) and `sub_40AAE3`'s fade dispatch.
                            // Its TRUE address (this struct's header comment has the full
                            // correction story) lands here with ZERO gap after
                            // in_conversation, matching 2011's exact declared adjacency
                            // "swap_portrait_lastchar; seperate_music_lib; in_conversation;
                            // screen_tint;" across all four fields with zero drift.
  char _pad_unexplored7[0x22]; // +0x8B0..0x8D2 (34 bytes), CONFIRMED NOT GameState territory
                            // -- occupied by `comparetonum`/`compareto` (both already IDA-
                            // named globals, XREF'd from `ParseText`/`Said`, core text-parser
                            // state unrelated to GameState) plus unlabeled bytes. This means
                            // 2011's declared `num_parsed_words`/`parsed_words[MAX_PARSED_
                            // WORDS]` (which 2011 places immediately before bad_parsed_word,
                            // below) are CONFIRMED ABSENT here -- no room for them, adjacent
                            // unrelated parser globals occupy the space instead.
  char bad_parsed_word[100];   // +0x8D2..0x936, high confidence: `SaidUnknownWord` (already
                            // matched, mechanical) does "strcpy(buffer, byte_4EF2EA); if
                            // (byte_4EF2EA[0]==0) ..." matching 2011's "strcpy(buffer,
                            // play.bad_parsed_word); if (play.bad_parsed_word[0]==0) ..."
                            // (AC.CPP:18038-18041) exactly.
  char _pad_align8[0x02];      // +0x936..0x938, compiler alignment padding (not a real field)
                            // -- 0x8D2 isn't 4-byte aligned, so a 100-byte array starting
                            // there ends 2 bytes short of the boundary the next field
                            // (raw_color, an int) needs; boxed in with zero slack by
                            // bad_parsed_word's own confirmed end and raw_color's
                            // independently-confirmed start.
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
// ExecutingScript, FullAnimation), just applied to array-of-structs vs. structure-of-arrays
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
// scale computation, dword_4CD2E8[]/dword_4E787C[], are `spritewidth[]`/`spriteheight[]`
// (both well-known AGS globals) -- HIGH confidence (UPGRADED, found during the ScreenOverlay
// round): `CreateGraphicOverlay` (already matched) passes them as create_bitmap_ex's height/
// width args, matching 2011's "create_bitmap_ex(final_col_dep, spritewidth[slott],
// spriteheight[slott]);" (AC.CPP:13128) exactly -- a second, independent usage context beyond
// the original width/height-scaling lead.
//
// xwas/ywas (2011's half-move-smoothing pair for scaled/zoomed character movement, read/written
// by `wantMoveNow(int,CharacterInfo*)`, AC.CPP:6349-6399) are CONFIRMED ABSENT from this build,
// not merely unfound: `char_zoom` (word_4EF1BC, the one field `wantMoveNow` would need to read)
// has EXACTLY TWO xrefs in the ENTIRE binary, both inside `prepare_characters_for_drawing`'s own
// zoom-scaling code -- no other function reads it, ruling out a separate `wantMoveNow`-equivalent
// function existing anywhere. Consistent with an earlier round's search for the INVALID_X
// (0x7530/30000) sentinel constant across the whole disassembly, which turned up only one
// coincidental unrelated hit (`add_screen_overlay`'s own overlay-position tracking, a reuse of
// the same generic sentinel value in a totally different subsystem) and zero genuine
// xwas/ywas-shaped hits. `invorder[MAX_INVORDER]`/`invorder_count` (2011's PER-CHARACTER
// inventory-order pair, also declared in `CharacterExtras`) are likewise CONFIRMED ABSENT here:
// this build's inventory-order tracking is `play_invorder`, a single GAME-WIDE array (see
// GameState's own `_pad_invorder_maybe` comment above), and `update_invorder` (already matched)
// is a genuinely simpler single-character predecessor with no per-character loop at all --
// the whole per-character-invorder feature this pair belongs to doesn't exist yet.
//
// `animwait` is CONFIRMED ABSENT as a separate field, resolved via a full read of `update_stuff`
// (already matched): every one of 2011's `charextra[aa].animwait` reads/writes (the
// walking<1/animwait=0 reset, the animwait>0/animwait-- decrement, and the final
// animwait=views[...].frames[...].speed+chi->animspeed computation, AC.CPP:6626-6653) operate on
// the SAME already-confirmed `CharacterInfo.wait`@+0x1C -- which ALSO independently serves 2011's
// `walkwait` role (the TURNING_AROUND-branch decrement, AC.CPP:6528) in the very same function.
// This build has ONE consolidated `wait` field doing the job of THREE separate 2011 fields
// (lip-sync wait, walkwait, animwait), matching `OldCharacterInfo`'s single declared `wait`
// (`acroom.h:2604`, no separate `walkwait` field in this ancestor at all) -- a real, structural
// simplification, not a gap in the evidence. Bonus: this same read upgrades
// `CharacterInfo.animspeed`@+0x42 from tentative to HIGH confidence (directly read as the second
// addend in the animwait computation).
//
// `process_idle_this_time` is CONFIRMED to exist conceptually but NOT as a per-character array:
// `update_stuff`'s "(loopcounter%40==0) || (charextra[aa].process_idle_this_time==1)" gate
// (AC.CPP:6867) matches disasm's "dword_523120 % 0x28(40)==0 OR dword_52320C==1" exactly --
// identifying `dword_523120` as a new global, `loopcounter` (bonus, not independently verified
// beyond this one modulo-40 match), and `dword_52320C` as this build's `process_idle_this_time`
// equivalent, but implemented as a SINGLE GLOBAL flag: set inside the walking<1 branch of the
// per-character loop, reset to 0 once before a second per-character loop begins (rather than a
// 50-entry per-character array). Works because, in this build's loop structure, the flag is only
// ever set and consumed within the processing of the SAME character, never carried across to a
// different character's turn -- a genuine simplification made possible by this build's flatter
// single-pass-per-character loop shape, not a bug or a gap.
//
// `slow_move_counter` is a LOW-VALUE, likely-unconfirmable lead: even in 2011's OWN source, this
// field is written exactly once (zeroed at game startup, AC.CPP:26259) and never read or written
// anywhere else in the entire file -- effectively dead weight even in the reference build itself.
// No behavioral evidence could distinguish "this build has it too" from "this build never had
// it" for a field neither build's own code actually uses. Left genuinely open, not worth
// further search time.
//
// `tint_r`/`tint_g`/`tint_b`/`tint_level`/`tint_light` (per-character tint override, gated by
// `CHF_HASTINT`=`0x2000`, AC.CPP:8319-8327) are CONFIRMED ABSENT (UPGRADED from medium-confidence
// "likely absent" -- see struct-layout-drift.md for the full follow-up round): tracing
// `prepare_characters_for_drawing`'s actual scale-then-blit control flow for the character-sprite
// path end to end -- from the confirmed zoom-scaling code (AC.CPP:8307-8317) through bitmap
// creation/`clear_to_color`, the `ViewFrame272.flags&1` mirroring check, the
// `SpriteCache::operator[]` sprite fetch, to the final `render_to_screen`/blit -- shows NO
// tint-related step anywhere in that sequence: no call shaped like 2011's `get_local_tint(x,y,
// noLighting,&amount,&r,&g,&b,&light,&level)` (an 8-argument call, AC.CPP:7661-7737) or
// `apply_tint_or_light(...)` (AC.CPP:7741+) appears between the zoom-scaling code and the final
// blit. This is stronger than the earlier "zero `0x2000` literal" evidence alone -- it's not just
// the `CHF_HASTINT` branch that's missing, the WHOLE surrounding tint-computation-and-application
// subsystem (both the per-character-override branch AND the ambient/room-tint fallback branch) is
// absent from this code path. Reinforced by neither `get_local_tint` nor `apply_tint_or_light`
// being matched, or even flagged as an unmatched lead, anywhere else in the whole binary. `play`'s
// own `rtint_red`/`rtint_green`/`rtint_blue`/`rtint_level`/`rtint_light` fields (`get_local_tint`'s
// own room-tint-override source, `acruntim.h:583`) are CONFIRMED ABSENT too (UPGRADED from
// "remain unconfirmed, consistent" -- checked directly this round): these fields' only writer,
// `SetAmbientTint(int,int,int,int,int)` (`AC.CPP:2615-2629`, a DISTINCT script API function from
// the already-matched `TintScreen`), has zero occurrences of its distinctive error string
// (`"!SetTint: invalid parameter..."`) anywhere in the extracted string dataset -- neither its
// reader (`get_local_tint`, already shown absent) nor its writer (`SetAmbientTint`) exist in this
// build. `TintScreen` (already matched, writing the separate `screen_tint` field) is this build's
// only screen/room-tint mechanism -- a simpler, older, palette-manipulation-based approach that
// predates the RGB/opacity/luminance-based `SetAmbientTint`/`get_local_tint`/`apply_tint_or_light`
// subsystem entirely.

struct ScreenOverlay {
  // FRESH SURVEY -- this build's version, recovered in a single round from `add_screen_overlay`
  // (already matched, Common/AC.CPP:3451-3474) and `find_overlay_of_type` (new match this
  // round, AC.CPP:3443-3449). Array-of-structs (unlike the CharacterExtras precedent just
  // above), base `dword_4CD220`, stride 0x14 (20 bytes) -- one field per one assignment
  // statement in add_screen_overlay's construction sequence, zero ambiguity. Capacity 10
  // (checked directly against a literal in add_screen_overlay), vs. 2011's declared
  // MAX_SCREEN_OVERLAYS=20 (Common/acruntim.h:841) -- the usual 2x-reduction pattern seen
  // throughout this project. CONFIRMED ABSENT vs. 2011's current declaration
  // (Common/acruntim.h:272-280): `bmp` (IDriverDependantBitmap*, a later hardware-acceleration
  // abstraction this build predates, same pattern as CharacterInfo.actx/acty),
  // `bgSpeechForChar`, `associatedOverlayHandle`, `hasAlphaChannel`,
  // `positionRelativeToScreen` -- this build's struct is exactly 2011's first 5 fields
  // (pic/type/x/y/timeout) and nothing more.
  block pic;                    // +0x00, high confidence: "screenover[numscreenover].pic =
                            // piccy;" -- direct assignment from add_screen_overlay's own
                            // `piccy` parameter.
  int type;                     // +0x04, high confidence: "screenover[numscreenover].type =
                            // type;" -- also independently confirmed via
                            // find_overlay_of_type's own field read.
  int x;                        // +0x08, high confidence: direct assignment from
                            // add_screen_overlay's `x` parameter.
  int y;                        // +0x0C, high confidence: direct assignment from
                            // add_screen_overlay's `y` parameter.
  int timeout;                  // +0x10, high confidence: "screenover[numscreenover].timeout
                            // = 0;" -- set to a literal 0 at creation time, matching source
                            // exactly.
};

// Related globals found in the same round (not ScreenOverlay struct members):
//   int numscreenover;   // dword_52318C, high confidence: the count/next-index global,
//                         // checked against the literal capacity (10) and incremented after
//                         // each new overlay -- matches 2011's `numscreenover` exactly.
//   int is_complete_overlay; // dword_523168, high confidence: incremented when type==
//                         // OVER_COMPLETE(2), matching 2011's "if (type==OVER_COMPLETE)
//                         // is_complete_overlay++;" exactly.
//   int is_text_overlay;     // dword_52316C, high confidence: incremented when type==
//                         // OVER_TEXTMSG(1), matching 2011's "if (type==OVER_TEXTMSG)
//                         // is_text_overlay++;" exactly -- a SECOND independent confirmation,
//                         // having already surfaced in an earlier GameState round via
//                         // check_controls (see that entry).

struct sprstruc {
  // Matches 2011's declared `sprstruc` (`Common/acroom.h:181-198`) exactly: `short sprnum, x, y,
  // room, on;` (5 packed shorts, `PCKD`, no padding), used by `RoomStruct.sprs[10]` below (this
  // build's own already-independently-confirmed 10-byte-per-element stride, from `load_main_block`
  // memsetting/freading the whole `sprs[]` region as one 100-byte block -- see that field's own
  // entry). Never formalized as its own type until this round.
  //
  // Confirmed via `sub_424750` (RESOLVED, previously an unnamed helper -- see its own matches.json
  // entry), the per-element constructor callback for `RoomStruct.sprs[]`'s C++ array-of-objects
  // default-construction (`vector constructor iterator`, called from `roomstruct::roomstruct()`
  // with ElementSize=0xA(10)/Count=0xA(10) matching `sprs[10]`'s own address/stride/capacity
  // exactly): its ENTIRE body is "[this+8] = 0" (a single 2-byte write) -- matching 2011's own
  // `sprstruc() { on = 0; }` (`acroom.h:186`) precisely, both in VALUE and in being the constructor's
  // only action. Since `on` is 2011's declared 5th/LAST field (`sprnum`@0, `x`@2, `y`@4, `room`@6,
  // `on`@8, each a natural 2-byte short with zero padding needed), this single confirmed write
  // pins down the whole 10-byte layout with zero slack -- the same "anchored from one confirmed
  // field plus the already-independently-confirmed total size" reasoning used elsewhere in this
  // project (e.g. `MouseCursor.name[10]`).
  short sprnum;                  // +0x00, medium-high confidence: positional, boxed in by the
                            // confirmed total stride and `on`'s own confirmed position -- no
                            // direct disassembly access to this specific field found yet.
  short x;                       // +0x02, medium-high confidence: see `sprnum` above.
  short y;                       // +0x04, medium-high confidence: see `sprnum` above.
  short room;                    // +0x06, medium-high confidence: see `sprnum` above.
  short on;                      // +0x08, high confidence: directly confirmed via
                            // `sprstruc__sprstruc`'s exact constructor-literal match (see
                            // struct-level comment above).
};

struct PolyPoints {
  // Matches 2011's declared `PolyPoints` (`Common/acroom.h:251-264`) exactly: `int x[MAXPOINTS=30];
  // int y[MAXPOINTS=30]; int numpoints;` (61 ints, 244/0xF4 bytes total). This is
  // `RoomStruct.wallpoints[15]`'s element type (see that field's own entry) -- a walkable area's
  // raw polygon vertex list, used only by the AGS EDITOR for room authoring; the runtime engine
  // itself never reads individual vertices, only the pre-rasterized `walls` bitmap mask, so no
  // already-matched RUNTIME function was expected to reference these fields directly (and indeed
  // none does -- `wallpoints` has zero references anywhere in `Engine/`, only in `Common/acroom.h`'s
  // room-file I/O code).
  //
  // Total size (0xF4) was first confirmed via `load_main_block`'s own `fread(rst+0x1574,
  // ElementSize=0F4h, Count=numwalkareas)` (see `RoomStruct.wallpoints`'s own entry). A SECOND,
  // fully independent confirmation of both the total size AND the internal `numpoints` field
  // position came from `sub_4247D0` (RESOLVED, previously unnamed -- see its own matches.json
  // entry), the per-element constructor callback for `wallpoints[]`'s own C++ array-of-objects
  // default-construction (called from `roomstruct::roomstruct()` with ElementSize=0F4h(244)/
  // Count=0Fh(15), matching `wallpoints[15]`'s address/stride/capacity exactly): its ENTIRE body is
  // "[this+0F0h] = 0" (a single 4-byte write) -- matching 2011's own `PolyPoints() { numpoints = 0;
  // }` (`acroom.h:264`) precisely. `x[30]`/`y[30]` themselves are NOT independently confirmed at
  // the individual-element level (no access site exists anywhere in this build), but are boxed in
  // with zero slack: `numpoints`'s own confirmed offset (`+0xF0`=240=30 ints + 30 ints) is exactly
  // where 2011 declares it, immediately after both 30-int arrays, and no other layout of "two
  // 30-element int arrays plus a trailing int, totaling 244 bytes" is possible without either array
  // itself changing size -- which would move `numpoints`'s own confirmed offset.
  int x[30];                     // +0x000..0x78 (120 bytes), medium-high confidence: boxed in by
                            // `numpoints`'s own confirmed offset and the struct's own confirmed
                            // total size -- see struct-level comment above.
  int y[30];                     // +0x078..0xF0 (120 bytes), medium-high confidence: see `x` above.
  int numpoints;                  // +0xF0, high confidence: directly confirmed via
                            // `PolyPoints__PolyPoints`'s exact constructor-literal match (see
                            // struct-level comment above).
};

struct RoomStruct {
  // FRESH SURVEY, ROUND 1 -- 2011's `roomstruct` (Common/acroom.h:806, the current-room data
  // format: walkable areas, hotspots, regions, background scenes, etc.) has never been formalized
  // in this project before, despite being referenced incidentally in several already-matched
  // functions' evidence (calculate_max_volume's thisroom.options[ST_VOLUME], etc.). This build's
  // global instance is ALREADY IDA-named `rstruc` (not a name from this project -- pre-existing,
  // confirmed live via `load_room`'s "push offset rstruc" call argument, already matched).
  // UPDATE (several rounds later): this struct ALSO had 4 pre-existing IDA field names
  // (`rstruc.walls/object/lookat/regions`) that looked at first like the same "already recovered,
  // just needs formalizing" situation as `SpriteCache` -- that turned out to be WRONG (see the
  // MAJOR CORRECTION note attached to the real leading fields below): those names came from IDA's
  // own unverified `roomstruct <?>` type, not independent confirmation, and every one of them was
  // off by 4 bytes. Left here as a cautionary note for the pattern itself, not just this struct.
  //
  // IMPORTANT CAUTION: IDA's Local Types library ALSO already has a type literally named
  // `roomstruct` applied to this global ("rstruc roomstruct <?>"). That type is UNVERIFIED
  // against this build and must NOT be trusted wholesale -- it is almost certainly either a
  // blind import of 2011's declared layout, or an old placeholder. This project has ALREADY shown
  // (independently, via RoomStatus/RoomObject/GameState work in earlier rounds) that this
  // struct's own capacity constants drift substantially smaller here than 2011's declarations
  // (MAX_INIT_SPR=10 not 2011's larger value, MAX_HOTSPOTS=20 not 2011's 50, MAX_WALK_AREAS=15
  // matching MAX_WALK_AREAS+1=16 exactly) -- the same "never trust a 2011 layout without
  // independent verification" caution that applies to every other struct in this project applies
  // doubly hard here, since a pre-existing IDA type makes it tempting to skip that step. Every
  // field below is verified independently, exactly like everywhere else in this codebase.
  //
  // Evidence source for this round: `load_main_block` (already matched, `Common/acroom.h:1605`,
  // called from `load_room`/already matched) -- AGS's own room-file-loading code, which inits/
  // reads roomstruct's fields in a strict, traceable sequence, the same kind of anchor
  // `count_data_offsets.py` used for `GameSetupStructBase`'s .data-section layout.
  //
  // MAJOR CORRECTION (found several rounds later, while tracing load_main_block's background-
  // picture-loading calls): the four leading fields below were originally recorded as
  // `walls`@+0x00/`object`@+0x04/`lookat`@+0x08/`regions`@+0x0C, "confirmed" via the pre-existing
  // IDA field names `rstruc.walls`/`.object`/`.lookat`/`.regions` appearing directly in
  // `load_room`'s disassembly. That was NOT independent evidence -- it was IDA's own
  // ALREADY-FLAGGED-AS-UNVERIFIED `roomstruct <?>` type (see the caution above) rendering itself
  // through the disassembly's symbolic STRUCT.FIELD display, exactly the trap this same comment
  // warned against. The actual raw-offset evidence (found by reading `load_main_block`'s
  // `loadcompressed_allegro` calls, which reference `rst+0x4`/`rst+0x8`/`rst+0xC`/`rst+0x10` via
  // literal hex arithmetic on the `rst` PARAMETER -- untyped, so NOT going through IDA's
  // unverified struct at all, genuinely independent) shows every one of those four fields sits 4
  // bytes LATER than previously recorded, with a fifth, previously-unknown field -- `ebscene[0]`
  // -- actually occupying +0x00. `apply_structs.py`'s earlier entries are corrected in place
  // below rather than silently rewritten; see `struct-layout-drift.md` for the full writeup.
  char _pad_unknown_0[4];        // +0x00, role NOT confirmed -- CORRECTION (found this round,
                            // reading `load_room`'s own cleanup code): the previous round
                            // recorded this field as `ebscene[0]` itself, reasoning from
                            // `load_main_block`'s "sub_40365D(Stream,Block=[rst],
                            // Buffer=rst+0x14); [rst]=dword_4EDA3C;" sequence (matching source's
                            // "tesl=load_lzw(opty,rstruc->ebscene[0],rstruc->pal);
                            // rstruc->ebscene[0]=recalced;", `acroom.h:1926-1927`). That
                            // reasoning is now shown to be INCOMPLETE: `load_room` (the caller,
                            // already matched)'s own room-cleanup loop destroys
                            // `[rstruc+c*4+0x3A0C]` for `c=1..num_bscenes-1`, matching source's
                            // "destroy_bitmap(rstruc->ebscene[c]); rstruc->ebscene[c]=NULL;"
                            // exactly -- proving `ebscene[]`'s REAL, PERSISTENT array base is
                            // `+0x3A0C` (see that field below), not `+0x00`. `dword_4EDA3C`
                            // (`recalced`) still gets copied FROM `+0x00` INTO `+0x3A0C`
                            // immediately after the `load_lzw` call (`[rst+0x3A0C]=[rst]`,
                            // confirmed in `load_main_block`'s own body) -- so `+0x00` is
                            // genuinely READ and WRITTEN in this exact sequence, just not as a
                            // persistent `ebscene[0]` slot; it's used as a transient holding spot
                            // (plausibly `load_lzw`'s own "existing bitmap" input template, or
                            // simply reused scratch space) whose value gets moved to its real
                            // destination immediately after.
                            //
                            // FOLLOW-UP (this round, `load_new_room` -- already matched):
                            // strengthens the "transient cache" picture without fully closing it.
                            // After a resolution-mismatch-driven resize loop over every
                            // `ebscene[c]` entry (bounded by the confirmed `num_bscenes`, gated
                            // on the confirmed `resolution` no longer matching
                            // `current_screen_resolution_multiplier_x`), `load_new_room` does
                            // "mov edx,[ebscene-array-base]; mov [rstruc+0],edx;" -- i.e.
                            // `[rstruc+0] = ebscene[0]`, an EXPLICIT, direct assignment of
                            // `ebscene[0]`'s (possibly just-resized) value into this field. This
                            // confirms `+0x00` really does function as a working cache of "the
                            // currently active background bitmap" (refreshed here after every
                            // room load/resize), consistent with -- and reinforcing -- the
                            // transient-holding-spot theory from `load_main_block`'s own
                            // sequence, but its "official" 2011 identity (if it maps to a
                            // declared field at all, rather than being scratch state this build
                            // added on top of `roomstruct`) is still not established. Left as an
                            // honestly-unconfirmed pad rather than guessed at a third time.
                            //
                            // FURTHER SUPPORTING EVIDENCE (found this round, `roomstruct__
                            // roomstruct` -- this build's newly-matched `RoomStruct` default
                            // constructor): zeroes `+0x00` in the SAME instruction group as
                            // `walls`/`object`/`lookat` ("[rst]=0; [rst+4]=0; [rst+8]=0;
                            // [rst+0xC]=0;", four consecutive NULLs) -- interesting because 2011's
                            // own constructor's very FIRST statement is "ebscene[0] = NULL; walls
                            // = NULL; object = NULL; lookat = NULL;" (`acroom.h:879`), a genuine,
                            // still-unexplained REDUNDANCY in 2011's own source (`ebscene[0]` is
                            // set to NULL a SECOND time later, at `acroom.h:889`, right next to
                            // `num_bscenes=1`). This build's constructor zeroing `+0x00` in
                            // lockstep with `walls`/`object`/`lookat` -- rather than alongside
                            // `num_bscenes`/`bscene_anim_speed`/`bytes_per_pixel`, where the REAL,
                            // persistent `ebscene[]` array (`+0x3A0C`) gets its own separate zero
                            // -- is consistent with `+0x00` being this build's counterpart to
                            // 2011's own first, redundant `ebscene[0]=NULL` line specifically, not
                            // the second one. Suggestive, not decisive: still doesn't independently
                            // confirm `+0x00`'s identity beyond "transient cache of the active
                            // background bitmap," but does explain WHY 2011's source shows that
                            // particular redundancy at all -- it's a vestige of exactly this kind
                            // of transient/duplicate field once existing where `+0x00` still does
                            // here.
                            //
                            // FOLLOW-UP (this round): a structural asymmetry between how scene 0
                            // vs. scenes 1+ get loaded offers a plausible WHY for the staging
                            // behavior, still without fully confirming it. `load_room`'s own
                            // animated-background loop (`BLOCKTYPE_ANIMBKGRND`, scenes 1..
                            // num_bscenes-1, already matched) reads/writes `ebscene[c]` DIRECTLY
                            // at `+0x3A0C+c*4` for every c>=1, with zero staging through `+0x00`
                            // anywhere -- e.g. "mov eax,[edx+ecx*4+3A0Ch]; push eax /*Block*/;
                            // ...; mov [eax+edx*4+3A0Ch],ecx" for the load, and an exactly
                            // parallel direct-addressed sequence for the pre-load destroy loop.
                            // Only scene 0's own load (inside `load_main_block`) goes through
                            // `+0x00` first and copies the result into `+0x3A0C` afterward. If
                            // `+0x00` really is a fast-access cache read by drawing code
                            // specifically for "the currently active background" (per
                            // `load_new_room`'s own cache-refresh evidence above), this asymmetry
                            // makes sense: only scene 0 is drawn every frame by default, so only
                            // its loader bothers routing through the would-be cache slot and
                            // keeping it in sync -- the other, less-frequently-displayed animated
                            // frames have no reason to. No direct READER of `+0x00` by any drawing
                            // function has been found yet to clinch this, so the identity still
                            // isn't independently confirmed -- recorded as a plausible unifying
                            // explanation for two previously-separate pieces of evidence, not a
                            // new confirmation.
                            //
                            // CORRECTION (immediate follow-up, same investigation thread): the
                            // "read by drawing code" half of that theory doesn't hold up.
                            // `RawSaveScreen`/`RawRestoreScreen`/`RawDrawImage` (all already
                            // matched) are this build's actual raw-screen-drawing API, and all
                            // three read the CURRENTLY DISPLAYED background via
                            // "dword_523094[dword_4EEB58*4]" -- `dword_4EEB58` is the already-
                            // confirmed `GameState.bg_frame`, and `dword_523094` (proven via a
                            // decisive, zero-slack arithmetic chain: `dword_523088`/`52308C`/
                            // `523090`/`523094` sit at consecutive +4-byte offsets, exactly
                            // matching the already-confirmed `num_bscenes`@+0x3A00/
                            // `bscene_anim_speed`@+0x3A04/`bytes_per_pixel`@+0x3A08/`ebscene[0]`@
                            // +0x3A0C) IS `ebscene[0]`, accessed via IDA's own auto-generated
                            // standalone-global name because `rstruc`'s applied IDB type doesn't
                            // yet extend far enough to resolve it as a struct-relative access.
                            // Matches 2011's own `RAW_START` macro ("abuf=thisroom.
                            // ebscene[play.bg_frame]", `AC.CPP:14355`) exactly. This means the
                            // ACTUAL drawing code reads `ebscene[bg_frame]` straight from
                            // `+0x3A0C+bg_frame*4` with ZERO reference to `+0x00` anywhere --
                            // there is no confirmed drawing-code reader of `+0x00` after all, and
                            // the "fast-access cache for the currently-displayed background, read
                            // by drawing code" framing above should be read with that caveat: the
                            // one class of function best positioned to be that reader turns out
                            // not to touch `+0x00` at all. `+0x00`'s identity remains exactly as
                            // unconfirmed as before -- this doesn't reopen anything already
                            // closed, since `ebscene[]`'s own offsets are now confirmed a THIRD
                            // independent way, but it does retract the specific "drawing code
                            // reads the cache" support offered for the unifying theory above.
  block walls;                  // +0x04, high confidence (CORRECTED from +0x00): the last of
                            // three UNCONDITIONAL, consecutive `loadcompressed_allegro`
                            // (`sub_403846`, 4-arg: FILE*, block* by ADDRESS, color*, long --
                            // matching 2011's own `loadcompressed_allegro` signature) calls
                            // targeting `rst+0x4`/`rst+0x8`/`rst+0xC` in that exact order matches
                            // source's own unconditional trio "loadcompressed_allegro(opty,
                            // &rstruc->walls,...); loadcompressed_allegro(opty,&rstruc->object,
                            // ...); loadcompressed_allegro(opty,&rstruc->lookat,...);"
                            // (`acroom.h:1946-1952`) exactly, in the same relative order.
                            // SECOND, INDEPENDENT confirmation (found this round, via USAGE
                            // rather than load order for the first time): `sub_40AD11` (newly
                            // matched this round, see its own matches.json entry) is
                            // `redo_walkable_areas()` -- confirmed beyond doubt via its
                            // `walkable_areas_on[]`-gated pixel-clearing loop matching 2011's
                            // "if(play.walkable_areas_on[thisroom.walls->line[hh][ww]]==0)
                            // _putpixel(thisroom.walls,ww,hh,0);" (`AC.CPP:3703-3718`) exactly --
                            // and it operates on THIS field (`+0x04`), matching 2011's own
                            // `thisroom.walls` target precisely. A second, independent site
                            // (`load_new_room`'s room-entry walkable-position check, testing the
                            // character's entry point against this same mask) reinforces it.
                            //
                            // IMPORTANT CAVEAT, applying to ALL FOUR of `walls`/`object`/
                            // `lookat`/`regions` below: at the time of writing, every access to
                            // the GLOBAL `rstruc` via its own applied struct type (as opposed to
                            // the raw-offset pointer-parameter form `load_main_block`/`load_room`
                            // use, which is what actually established these offsets and is
                            // unaffected) displays in the currently-exported `rob_blanc_1.asm`
                            // with each field name shifted ONE POSITION EARLY relative to this
                            // file's own doubly-confirmed order: `rstruc.walls` (displayed)
                            // resolves to the REAL `+0x00` (the still-unconfirmed `ebscene[0]`-
                            // cache field -- `load_new_room`'s own "mov rstruc.walls,
                            // dword_523094(ebscene[0])" is exactly the already-documented
                            // `+0x00` cache-refresh line, now cross-confirmed via this shift);
                            // `rstruc.object` (displayed) resolves to REAL `+0x04` (=`walls`,
                            // per `redo_walkable_areas` above); `rstruc.lookat` (displayed)
                            // resolves to REAL `+0x08` (=`object`/walk-behind, confirmed via
                            // `sub_410631`'s `walkbehind_base[]`-indexed pixel check, itself
                            // called from `prepare_characters_for_drawing`); `rstruc.regions`
                            // (displayed) resolves to REAL `+0x0C` (=`lookat`/hotspot mask,
                            // confirmed via `get_hotspot_at`, already matched, reading it via
                            // `getpixel` for its own hotspot lookup). Four independent usage
                            // sites, four confirmations, all consistent with one uniform
                            // "off-by-one-field-early" stale type currently applied in the IDB --
                            // this file's own declared offsets/order are correct and now backed
                            // by BOTH load-order AND usage evidence; only the IDB's own type
                            // definition needs a fresh `apply_structs.py` run and re-export to
                            // catch up. Any future investigation reading `rstruc.FIELD` symbolic
                            // accesses in the CURRENT `.asm` export should mentally shift the
                            // displayed name one position early until that re-sync happens.
  block object;                  // +0x08, high confidence (CORRECTED from +0x04): see `walls`
                            // above -- the second of the same three-call trio, and see the same
                            // comment for the confirmed stale-display-shift caveat.
  block lookat;                  // +0x0C, high confidence (CORRECTED from +0x08): see `walls`
                            // above -- the third of the same three-call trio, and see the same
                            // comment for the confirmed stale-display-shift caveat.
  block regions;                 // +0x10, high confidence (CORRECTED from +0x0C): a FOURTH
                            // `loadcompressed_allegro` call, gated on room-file version>=8,
                            // targeting `rst+0x10`, sitting immediately BEFORE the unconditional
                            // walls/object/lookat trio in the disassembly's own call order --
                            // matches source's own version-gated `regions` load
                            // (`acroom.h:1938-1939`) coming before the walls/object/lookat trio
                            // in source order too.
  char pal[256][4];              // +0x14..0x414 (1024 bytes, 256 x 4-byte `RGB` entries), high
                            // confidence (NEW, closing the previous round's "1024 vs. 1028, a
                            // 4-byte remainder" mystery completely): the SAME address is passed
                            // as the shared palette-buffer argument to EVERY ONE of the four
                            // `loadcompressed_allegro`/`load_lzw` calls above, matching 2011's
                            // `rstruc->pal` being reused as the shared destination in every one
                            // of those same calls exactly. `RGB`={r,g,b,filler}=4 bytes
                            // (`allegro/palette.h:26-30`) x 256 = 1024 bytes, landing EXACTLY on
                            // `numobj`'s own independently-confirmed start (`+0x414`) with ZERO
                            // remaining slack -- the earlier round's 4-byte discrepancy was
                            // entirely explained by the leading-fields correction above, not a
                            // separate unresolved gap.
  short numobj;                  // +0x414, high confidence: `load_main_block`'s own
                            // "fread(rst+0x414, ElementSize=2, Count=1)" matches source's
                            // "fread(&rstruc->numobj, 2, 1, opty);" (`acroom.h:1650`) exactly --
                            // confirms both the field AND its `short` type (2011 declares it
                            // `short numobj;`, `acroom.h:810`). BONUS DRIFT (found via
                            // `roomstruct__roomstruct`, this round): the constructor's default
                            // value, "[rst+0x414]=0xF(15)", matches 2011's own constructor idiom
                            // "numobj = MAX_OBJ;" (`acroom.h:880`) in ROLE, but this build's own
                            // `MAX_OBJ`-equivalent default is 15, not 2011's declared `MAX_OBJ=16`
                            // (`acroom.h:60`) -- a one-off reduction, the same direction as this
                            // project's usual smaller-capacity pattern though unusually small in
                            // magnitude.
  short objyval[15];             // +0x416..0x434 (30 bytes, 2011's `short objyval[MAX_OBJ]`,
                            // walkbehind-area baselines), high confidence -- RESOLVED this round by
                            // connecting two previously-separate pieces of evidence that were never
                            // cross-referenced against each other. Start address/element type/size
                            // confirmed via `load_main_block`'s "fread(rst+0x416, ElementSize=2,
                            // Count=[rst+0x414])" matching source's "fread(&rstruc->objyval[0], 2,
                            // rstruc->numobj, opty);" (`acroom.h:1655`) exactly -- but that alone
                            // only bounds the READ to a dynamic `numobj`-driven count, not the
                            // array's own fixed CAPACITY, so the trailing 30 bytes were left as an
                            // unconfirmed pad pending direct evidence for this build's own `MAX_OBJ`
                            // value. That evidence has since been found (a later round,
                            // `roomstruct__roomstruct`'s own constructor default: "numobj@+0x414 =
                            // 0xF(15)", matching source's `numobj=MAX_OBJ;` idiom -- see `numobj`'s
                            // own entry) -- confirming this build's `MAX_OBJ`-equivalent is
                            // EXACTLY 15, which lands with ZERO remainder on this field's own
                            // already-established 30-byte (15-short) span. Both pieces of evidence
                            // were independently correct when found, just never connected until
                            // this round.
  short whataction[130];        // +0x434..0x538 (260 bytes), high confidence, MAJOR
                            // ARCHAEOLOGICAL FINDING: this build's room-file format STILL
                            // ACTIVELY READS the "obsolete v2.00 action editor" arrays that
                            // 2011's own header only keeps as legacy declarations
                            // (`whataction[NUM_CONDIT+3]`, `acroom.h:813`) -- `load_main_block`
                            // has a real, version-gated read path for room-file versions 7 and
                            // 8 (`arg_C>=7 && arg_C<9`) that `fread`s this array directly, plus
                            // an even-older sub-v7 conversion path. Capacity 130 confirmed via a
                            // perfect zero-gap chain: this array's end lands exactly on `val1`'s
                            // start, which lands exactly on `val2`'s start, `otcond`'s start,
                            // and `points`'s start -- four consecutive zero-slack boundaries in
                            // a row, computing to 130 = `NUM_CONDIT+3` with `NUM_CONDIT=127`
                            // (matching the same-function's own `ElementCount=0x7F`(127) local
                            // used by the sub-v7 conversion path). Whether this 2002 build's OWN
                            // room files actually exercise this path (i.e. were compiled with a
                            // pre-v9 room format) or only carries the dead-but-still-compiled
                            // fallback code is not established -- either way, the array's own
                            // layout is now confirmed.
  short val1[130];               // +0x538..0x63C (260 bytes), high confidence: see `whataction`
                            // above for the shared evidence (zero-gap chain + version-gated
                            // read path). Matches 2011's declared adjacency
                            // ("whataction[...];val1[...];", `acroom.h:813-814`) exactly.
  short val2[130];               // +0x63C..0x740 (260 bytes), high confidence: same evidence.
  short otcond[130];             // +0x740..0x844 (260 bytes), high confidence: same evidence.
  char points[130];              // +0x844..0x8C6 (130 bytes, `char` not `short` -- matches
                            // 2011's declared `char points[NUM_CONDIT+3]`, `acroom.h:817`,
                            // exactly), high confidence: same evidence, closing the zero-gap
                            // chain -- ends exactly where `left` (below) begins.
  short left;                    // +0x8C6, high confidence: `load_main_block`'s version>=9
                            // path `fread`s `top`/`bottom`/`left`/`right` in that READ order
                            // (matching source's own read order exactly, `acroom.h:1704-1707`)
                            // into addresses that only make sense in DECLARED order left/right/
                            // top/bottom (`acroom.h:819`) -- `fread(rst+0x8CA,2,1)`(top),
                            // `fread(rst+0x8CC,2,1)`(bottom), `fread(rst+0x8C6,2,1)`(left),
                            // `fread(rst+0x8C8,2,1)`(right) -- both the read-order AND the
                            // declared-order cross-checks land on the same four addresses with
                            // zero ambiguity.
  short right;                   // +0x8C8, high confidence: see `left` above.
  short top;                     // +0x8CA, high confidence: see `left` above.
  short bottom;                  // +0x8CC, high confidence: see `left` above.
  short numsprs;                 // +0x8CE, high confidence: `fread(rst+0x8CE,2,1)` matches
                            // "fread(&rstruc->numsprs,2,1,opty);" (`acroom.h:1709`) exactly,
                            // sitting with zero gap immediately after `bottom`.
  short nummes;                   // +0x8D0, high confidence (UPGRADED from medium/positional-only
                            // via a direct behavioral confirmation, same session): fills the
                            // exact 2-byte gap between `numsprs` and `sprs[]` matching 2011's
                            // declared adjacency ("short numsprs,nummes;", `acroom.h:820`) with
                            // zero slack, AND `fread(rst+0x8D0, ElementSize=2, Count=1)` --
                            // matching source's `fread(&rstruc->nummes,...)` role directly (read
                            // later in the file-loading sequence than its declared position,
                            // same "code order need not match struct order" pattern seen
                            // throughout this project) -- then consumed immediately after as the
                            // element count driving `message[]`'s own read loop, matching
                            // source's use of `nummes` as `message[]`'s bound exactly.
  sprstruc sprs[10];              // +0x8D2..0x936 (100 bytes), high confidence: `load_main_block`
                            // memsets this region to 0 (100 bytes) before conditionally
                            // `fread`ing it back. UPGRADED (this round) from a raw `short[10][5]`
                            // blob to a typed `sprstruc` array -- see `sprstruc`'s own declaration
                            // above for the constructor-level field confirmation
                            // (`sprstruc__sprstruc`, found via `roomstruct__roomstruct`'s own
                            // per-element array-construction call, ElementSize=0xA(10)/Count=0xA(10)
                            // matching this field's address/stride/capacity exactly -- a second,
                            // fully independent confirmation of both). Capacity 10 (100/10) --
                            // DRIFT vs. 2011's declared `MAX_INIT_SPR`, matching this build's
                            // already-established `RoomObject`/`objbaseline` array capacity of 10
                            // with zero further drift.
  // MAJOR STRUCTURAL FINDING: this build's field ORDER genuinely diverges from 2011's declared
  // order here, not just capacities. 2011 declares `objbaseline[MAX_INIT_SPR]` immediately after
  // `sprs[]`/`intrObject[]`/`objectScripts` (i.e. right here, at ~+0x936) -- but this build's own
  // `objbaseline` (confirmed above at +0x3858, via last round's 0xFF-fill memset) sits FAR AWAY,
  // AFTER the entire hotspot/walkarea block (`numwalkareas` through `misccond`). A bonus
  // reconfirmation ties the two finds together: `load_main_block`'s version>=9 path also does
  // "fread(rst+0x3858, ElementSize=4, Count=[rst+0x8CE])" -- i.e. reads INTO the already-
  // confirmed `objbaseline` address, using `numsprs`'s own value (not a fixed `MAX_INIT_SPR`
  // constant) as the read count -- a second independent confirmation of `objbaseline`'s address,
  // plus a genuine behavioral nuance (this build ties the object-baseline read-count to
  // `numsprs`, not a separate object count) worth remembering for later. `intrObject[]`/
  // `objectScripts` themselves are NOT found anywhere in this +0x936..+0x1570 gap or the
  // +0x936..+0x3858 gap explored so far -- CONFIRMED ABSENT (UPGRADED from plausible-by-
  // precedent, see the `scripts`/version-ceiling finding below): both are declared behind a
  // version>=21/version>=26 `deserialize_new_interaction`/`deserialize_interaction_scripts`
  // block that this build's `load_main_block` never compiled in at all (its complete set of
  // version-gate checks tops out at 14) -- not just consistent with the earlier
  // `NewInteraction`-predates-this-build finding, independently and decisively confirmed by it.
  char password[11];             // +0x936..0x941 (11 bytes), high confidence: `fread(rst+0x936,
                            // ElementSize=0xB(11), Count=1)` matches source's declared `char
                            // password[11];` (`acroom.h:829`) in both position and exact size --
                            // sits with zero gap immediately after `misccond`.
  char options[10];              // +0x941..0x94B (10 bytes), high confidence: `fread(rst+0x941,
                            // ElementSize=0xA(10), Count=1)` matches source's declared `char
                            // options[10];  // [0]=startup music` (`acroom.h:830`) in both
                            // position and exact size, sitting with zero gap immediately after
                            // `password`.
  char _pad_align_message[1]; // +0x94B..0x94C, compiler alignment padding (not a real field) --
                            // boxed in with zero slack by `options`'s own confirmed end and
                            // `message[]`'s own confirmed start (needs 4-byte alignment for its
                            // pointer array below).
  char *message[100];            // +0x94C..0xADC (400 bytes, 100 x 4-byte pointers), high
                            // confidence: a dedicated read loop (not a raw `fread`) decrypts
                            // each message string via `sub_403024` (an as-yet-unmatched
                            // decrypt-read helper, plausibly `read_string_decrypt`-adjacent --
                            // not confirmed this round), `malloc`s a buffer sized to the decoded
                            // string, and stores the pointer at "[rst+f*4+0x94C]" -- an exact
                            // structural match to 2011's declared `char *message[MAXMESS];`
                            // (`acroom.h:831`), including the pointer TYPE (this build does NOT
                            // drift to a fixed-size inline array here, unlike `hotspotnames`).
                            // Loop count is `nummes` (dynamic), but the RESERVED capacity is
                            // fixed at 100 -- confirmed via the zero-gap distance to `msgi[]`
                            // immediately below (100 x 4 bytes = 0x190, landing exactly on
                            // `msgi`'s own independently-confirmed start) -- `MAXMESS=100`.
  char msgi[100][2];              // +0xADC..0xBA4 (200 bytes, 100 x 2-byte `MessageInfo`
                            // entries), high confidence: `fread(rst+0xADC, ElementSize=2,
                            // Count=nummes)` for room-file version>=3, else
                            // `memset(rst+0xADC,0,0xC8(200))` -- both match 2011's declared
                            // `MessageInfo msgi[MAXMESS];` (`acroom.h:832`) exactly:
                            // `MessageInfo` itself is a packed 2-byte struct (`char displayas;
                            // char flags;`, `acroom.h:203-205`), represented here as a raw
                            // `short[100][2]`-shaped byte pair rather than declaring a new
                            // `MessageInfo` type. The message-loading loop directly confirms the
                            // SECOND byte's role too: "[rst+f*2+0xADD] |= 1" (a per-message
                            // fixup flag) matches `MessageInfo.flags`'s documented role exactly.
                            // The 200-byte memset fallback independently reconfirms
                            // `MAXMESS=100` (200/2) with zero drift from 2011's own constant.
  short wasversion;              // +0xBA4, high confidence: RESOLVED (closing the previously
                            // undifferentiated 32-byte `_pad_unexplored2b` gap). Confirmed via
                            // `load_room` (already matched, the CALLER of `load_main_block`, not
                            // `load_main_block` itself): "fread(&var_8,2,1,Stream);
                            // rstruc.wasversion=var_8; if (rstruc.wasversion<2 ||
                            // rstruc.wasversion>0Eh(14)) quit(\"Load_Room: Bad packed file. Either
                            // the file requires a newer or older version of...\")" -- an exact
                            // structural match to source's "rstruc->wasversion = rfh.version; if
                            // ((rstruc->wasversion<15)||(rstruc->wasversion>ROOM_FILE_VERSION))
                            // quit(\"Load_Room: Bad packed file...\");" (`acroom.h:2080-2088`),
                            // including the same error string. The bounds this build actually
                            // enforces (2..14) are LOWER on both ends than 2011's (15..29) --
                            // matching, and reinforcing from a brand new angle, round 8's
                            // capstone finding that this build's compiled engine never had code
                            // for room-format version 15+ at all; the floor of 2 additionally
                            // shows this build's minimum supported room format is even OLDER than
                            // 2011's own documented floor.
  short flagstates[15];          // +0xBA6..0xBC4 (30 bytes), MEDIUM confidence: positional/
                            // arithmetic-fit only -- no already-matched function reads this span
                            // directly. Sits with zero gap immediately after the now-confirmed
                            // `wasversion`@+0xBA4 and ends with zero remainder exactly at the
                            // already-confirmed `anims[10]` start (+0xBC4), matching 2011's
                            // declared "short wasversion; short flagstates[MAX_FLAGS];"
                            // adjacency (`acroom.h:833-834`) exactly, with `MAX_FLAGS=15`
                            // (`acroom.h:801`) matching `RoomStatus.flagstates`'s own already-
                            // confirmed capacity with zero drift. UPDATED HYPOTHESIS (this round,
                            // superseding the earlier "copied into RoomStatus.flagstates on first
                            // visit" guess): now that `RoomStatus.flagstates`'s own populating
                            // code IS known (`load_new_room`'s "for(chaa=0;chaa<15;chaa++)
                            // croom->flagstates[chaa]=0;" -- an unconditional RESET, not a copy
                            // from anywhere), the "source copy -> runtime copy" relationship
                            // already established for `hscond`/`objcond`/`misccond` does NOT
                            // extend to `flagstates` -- there is no copy site to find, because no
                            // copy happens. Reinforced by an exhaustive check of 2011's own
                            // `Engine/` source: `thisroom.flagstates`/`rstruc->flagstates` has
                            // ZERO usages anywhere -- this field is genuinely unused/dead weight
                            // even in the 2011 reference build, the same "declared but never read"
                            // status as `MouseCursor.name`/`GameSetupStructBase.target_win`
                            // elsewhere in this project. Left at MEDIUM (positional fit) since no
                            // negative search of THIS build's own disassembly has been done yet,
                            // but this is now a much weaker lead than previously framed.
  FullAnimation anims[10];       // +0xBC4..0x154C (2440 bytes, 10 x 244-byte `FullAnimation`
                            // entries), high confidence: `fread(rst+0xBC4, ElementSize=0xF4(244),
                            // Count=numanims)` for room-file version>=6, else
                            // `memset(rst+0xBC4,0,0x988(2440))` -- both match 2011's declared
                            // `FullAnimation anims[MAXANIMS];` (`acroom.h:835`) in position;
                            // capacity 10 (2440/244) -- CORRECTION (found later, reading
                            // `acroom.h:800`): `MAXANIMS` is ALREADY declared as `10` in 2011
                            // itself (`#define MAXANIMS 10`) -- this is a genuine ZERO-DRIFT
                            // match, not the smaller-capacity pattern common elsewhere in this
                            // project (an earlier version of this comment wrongly called it
                            // drift without having checked the actual 2011 constant value).
                            // UPGRADED from a raw byte blob to a typed `FullAnimation` array
                            // (found later, in the round that identified `FullAnimation` itself
                            // as this build's already-confirmed `GameAnimation` struct under its
                            // real 2011 name -- see `FullAnimation`'s own declaration above) --
                            // this field's own capacity (10) and immediately-following
                            // `numanims` field match 2011's declared "FullAnimation
                            // anims[MAXANIMS]; short numanims;" (`acroom.h:835-836`) adjacency
                            // with zero drift on both counts, an independent THIRD confirmation
                            // of the AnimationStruct/FullAnimation identification (alongside the
                            // size-arithmetic and field-semantics matches documented on
                            // `AnimationStruct`/`FullAnimation` themselves).
  short numanims;                // +0x154C, high confidence: read via `getw()` for version>=6
                            // (else defaulted to 0) directly into this address, immediately
                            // BEFORE the `anims[]` read that consumes it as the element count --
                            // matches 2011's declared adjacency ("anims[MAXANIMS]; short
                            // numanims;", `acroom.h:835-836`) positionally, though the READ order
                            // here is reversed (count read before the array it bounds, a common
                            // and unremarkable code-generation choice, not a struct-layout fact).
  short shadinginfo[16];         // +0x154E..0x156E (32 bytes), high confidence: `fread(rst+0x154E,
                            // ElementSize=2, Count=0x10(16))` for room-file version>=8, else
                            // `memset(rst+0x154E,0,0x20(32))` -- both match 2011's declared
                            // `short shadinginfo[16];  // walkable area-specific view number`
                            // (`acroom.h:837`) exactly, capacity 16 with zero drift. Sits with
                            // zero gap immediately after `numanims`.
  char _pad_align_walkareas[2]; // +0x156E..0x1570, compiler alignment padding (not a real
                            // field) -- boxed in with zero slack by `shadinginfo`'s own
                            // confirmed end and `numwalkareas`'s own already-confirmed start.
  int numwalkareas;             // +0x1570, high confidence: `load_main_block` inits this to the
                            // literal 0 ("rstruc->numwalkareas = 0;", `acroom.h:1614`), AND
                            // separately reads it via "fread(rst+0x1570, ElementSize=4, Count=1)"
                            // matching "fread(&rstruc->numwalkareas, 4, 1, opty);"
                            // (`acroom.h:1691`) exactly -- two independent confirmations.
  PolyPoints wallpoints[15];    // +0x1574..0x23C0 (3660 bytes total, 15 x 244-byte entries),
                            // RESOLVED (closing this project's single largest remaining RoomStruct
                            // gap, previously an undifferentiated 0xE4C-byte unknown span --
                            // earlier comment history: an initial guess that `left`/`right`/`top`/
                            // `bottom`/`numsprs`/`nummes`/`sprs[]` might live here was WRONG, those
                            // were later found at `+0x8C6..+0x936` instead; a later "STALE NOTE
                            // CORRECTED" pass then only ruled out `intrObject[]`/`objectScripts`
                            // without identifying the real contents). High confidence via TWO
                            // independent routes: (1) `load_main_block`: immediately after the
                            // already-confirmed `numwalkareas`@+0x1570 fread, `fread(rst+0x1574,
                            // ElementSize=0F4h(244), Count=[rst+0x1570])` matches
                            // `fread(&rstruc->wallpoints[0], sizeof(PolyPoints),
                            // rstruc->numwalkareas, opty);` (`acroom.h:1694`) exactly; (2)
                            // `roomstruct__roomstruct` (this build's `RoomStruct` default
                            // constructor, found this round -- see its own matches.json entry):
                            // its own C++ array-of-objects default-construction call targets
                            // `rst+0x1574` with ElementSize=0F4h(244)/Count=0Fh(15), matching this
                            // field's address/stride/capacity a SECOND, fully independent way. Both
                            // routes confirm the field NAME/position (2011's own `PolyPoints
                            // wallpoints[MAX_WALK_AREAS];`, `acroom.h:840`) and total capacity (15,
                            // matching 2011's `MAX_WALK_AREAS=15`, `acroom.h:250`, with ZERO drift
                            // -- unusual for this project, most fixed capacities shrink). UPGRADED
                            // (this round) from a raw byte blob to a typed `PolyPoints` array: the
                            // per-element constructor callback found via route (2), `sub_4247D0`
                            // (now `PolyPoints__PolyPoints`), directly confirms `PolyPoints.
                            // numpoints`@+0xF0 -- see `PolyPoints`'s own declaration above for the
                            // complete field-level writeup. Walkable-area polygon vertex data
                            // remains purely an AGS EDITOR/room-authoring concern at RUNTIME --
                            // `wallpoints` has zero references anywhere in `Engine/`, only in
                            // `Common/acroom.h`'s room-file I/O and constructor code, both now
                            // fully accounted for.
  int numhotspots;               // +0x23C0, high confidence: `load_main_block` inits this to the
                            // literal 0 ("rstruc->numhotspots = 0;", `acroom.h:1615`), AND
                            // separately reads it via "fread(rst+0x23C0, ElementSize=4, Count=1)"
                            // matching "fread(&rstruc->numhotspots, sizeof(int), 1, opty);"
                            // (`acroom.h:1659`) exactly -- two independent confirmations.
  short hswalkto[20][2];          // +0x23C4..0x2414 (80 bytes), high confidence (RESOLVED, closing
                            // last round's open pad): "fread(rst+0x23C4, ElementSize=4,
                            // Count=20)" matches source's "fread(&rstruc->hswalkto[0],
                            // sizeof(_Point), rstruc->numhotspots, opty);" (`acroom.h:1666`)
                            // exactly -- 2011's `_Point` is 4 bytes (2 packed shorts, matching
                            // this build's other `PCKD` conventions), represented here as a raw
                            // `short[2]` pair (x,y) rather than declaring a new `_Point` type,
                            // x20 = 80 bytes, capacity matching this build's already-confirmed
                            // `MAX_HOTSPOTS=20` with zero drift. Sits with zero gap immediately
                            // before `hotspotnames`.
  char hotspotnames[20][30];     // +0x2414..0x266C (600 bytes), high confidence, ARCHITECTURAL
                            // DRIFT from 2011: this build stores hotspot names as a FIXED
                            // INLINE `char[20][30]` array, not 2011's `char* hotspotnames[
                            // MAX_HOTSPOTS]` (a pointer array with each name individually
                            // `malloc`'d, `acroom.h:843`) -- `load_main_block` writes directly
                            // into "rst + f*0x1E + 0x2414" via `sprintf("Hotspot %d")`/
                            // `strcpy("No hotspot")` for f==0, with a 30-byte (0x1E) per-name
                            // stride matching the SAME function's own pre-v28-room-file-format
                            // fallback path later on ("hotspotnames[f]=malloc(30);
                            // fread(hotspotnames[f],30,1,opty);", `acroom.h:1681-1685`) -- this
                            // build keeps the OLD, fixed-30-byte-name convention inline rather
                            // than ever moving to heap-allocated variable-length names.
                            // Capacity 20 matches this build's already-confirmed
                            // `MAX_HOTSPOTS=20` (`RoomStatus.hotspot_enabled[20]`, several
                            // rounds ago) with zero drift. A SECOND, independent confirmation
                            // (this round): "fread(rst+0x2414, ElementSize=0x1E, Count=20)" in
                            // the SAME function's version-gated newer-format read path matches
                            // this exact address/stride/capacity too. Ends with ZERO gap exactly
                            // where `hscond` begins (`+0x266C`, see below).
  EventBlock hscond[20];         // +0x266C..0x31FC (2960 bytes), high confidence (RESOLVED,
                            // closing last round's open "1628-byte fully-byte-accounted-for"
                            // span): "fread(rst+0x266C, ElementSize=0x94, Count=20)" -- the
                            // ElementSize, 0x94(148), is an EXACT match to `EventBlock`'s own
                            // independently-confirmed total size (see that struct's own entry
                            // above) -- decisive, not just a size coincidence. This is
                            // `roomstruct`'s own SOURCE copy of the same per-hotspot
                            // EventBlock command-list array that `RoomStatus.hscond[20]`
                            // (already confirmed, several rounds ago) holds a per-save-slot
                            // RUNTIME copy of -- the room file stores the compiled command
                            // lists here, and they get copied into `RoomStatus` at room-load
                            // time. Capacity 20 matches `MAX_HOTSPOTS=20` exactly, matching
                            // `RoomStatus.hscond[20]`'s own already-confirmed capacity with
                            // zero drift. 2011 keeps the equivalent as a dead comment only
                            // (`/* EventBlock hscond[MAX_HOTSPOTS]; ... */`) -- this build's
                            // room-file format still actively uses it.
  EventBlock objcond[10];        // +0x31FC..0x37C4 (1480 bytes), high confidence: same evidence
                            // pattern as `hscond` immediately above -- "fread(rst+0x31FC,
                            // ElementSize=0x94, Count=10)" -- `roomstruct`'s own source copy of
                            // `RoomStatus.objcond[10]`'s per-object EventBlock list, capacity
                            // matching `MAX_INIT_SPR=10` exactly (this build's already-
                            // established `RoomObject`/`objbaseline` array capacity).
  EventBlock misccond;           // +0x37C4..0x3858 (148 bytes), high confidence: same evidence
                            // pattern again -- "fread(rst+0x37C4, ElementSize=0x94, Count=1)" --
                            // `roomstruct`'s own source copy of `RoomStatus.misccond`, a single
                            // room-wide EventBlock. Ends with ZERO gap exactly where
                            // `objbaseline` begins.
  int objbaseline[10];          // +0x3858..0x3880 (40 bytes), high confidence: `load_main_block`'s
                            // own 0xFF-fill memset ("memset(rst+0x3858, 0xFF, 0x28)") matches
                            // source's "memset(&rstruc->objbaseline[0], 0xff, sizeof(int)*
                            // MAX_INIT_SPR);" (`acroom.h:1619`) exactly -- the ONLY 0xFF-valued
                            // memset in the whole function, an unambiguous fingerprint. Capacity
                            // 10 (0x28/4) -- DRIFT vs. 2011's declared `MAX_INIT_SPR`, matching
                            // this build's already-established `RoomObject` array capacity of 10
                            // (several rounds ago) with zero further drift.
  short width;                   // +0x3880, high confidence: `load_main_block` inits this to the
                            // literal 320 (`0x140`), matching source's "rstruc->width = 320;"
                            // (`acroom.h:1611`) exactly.
  short height;                  // +0x3882, high confidence: inits to the literal 200 (`0xC8`),
                            // matching "rstruc->height = 200;" (`acroom.h:1612`) exactly.
  short resolution;              // +0x3884, high confidence: inits to the literal 1, matching
                            // "rstruc->resolution = 1;" (`acroom.h:1613`) exactly. SECOND
                            // confirmation (this round): `load_new_room` (already matched)
                            // compares this field against `current_screen_resolution_
                            // multiplier_x` to decide whether every `ebscene[c]` bitmap needs
                            // resizing after a room change -- matching 2011's own resolution-
                            // mismatch handling role exactly.
  short walk_area_zoom[16];       // +0x3886..0x38A6 (32 bytes), high confidence (RESOLVED,
                            // closing round 1's "ambiguous which is which" note): a count local
                            // (defaulted to 15, or read via `getw()` for version>=14, bounds-
                            // checked against a literal 16 with a "Too many walkable areas" quit
                            // matching source's `MAX_WALK_AREAS` check, `acroom.h:1839`) drives
                            // "fread(rst+0x3886, ElementSize=2, Count=NUMREAD)" for version>=10
                            // -- matches 2011's declared `short walk_area_zoom[MAX_WALK_AREAS+1];`
                            // (`acroom.h:856`) exactly, capacity 16 confirming `MAX_WALK_AREAS=15`
                            // with zero drift (already established elsewhere in this project).
  short walk_area_light[16];      // +0x38A6..0x38C6 (32 bytes), high confidence: same NUMREAD-
                            // driven read, "fread(rst+0x38A6, ElementSize=2, Count=NUMREAD)" for
                            // version>=13, matching 2011's declared `short
                            // walk_area_light[MAX_WALK_AREAS+1];` (`acroom.h:858`) exactly --
                            // sits with zero gap immediately after `walk_area_zoom`. 2011's
                            // intervening `walk_area_zoom2[MAX_WALK_AREAS+1]` (`acroom.h:857`)
                            // is CONFIRMED ABSENT here -- no room for it between the two
                            // confirmed zero-gap neighbors.
  char objectnames[10][30];      // +0x38C6..0x39F2 (300 bytes), high confidence (RESOLVED,
                            // closing the mystery left open since round 6): `load_room`
                            // (already matched)'s block-type dispatch loop -- a SEPARATE
                            // mechanism from `load_main_block`, handling `BLOCKTYPE_
                            // OBJECTNAMES`(5) -- does "if (fgetc(Stream) != [rstruc+0x8CE])
                            // quit(...); fread(rstruc+0x38C6, ElementSize=0x1E(30),
                            // Count=[rstruc+0x8CE]);" matching source's "if (fgetc(opty) !=
                            // rstruc->numsprs) quit(\"Load_room: inconsistent blocks for object
                            // names\"); fread(&rstruc->objectnames[0][0], MAXOBJNAMELEN,
                            // rstruc->numsprs, opty);" (`acroom.h:2133-2137`) exactly --
                            // `MAXOBJNAMELEN=30` (`acroom.h:802`) matches the `0x1E` element
                            // size exactly, and 300/30=10 matches this build's already-
                            // established `MAX_INIT_SPR=10` capacity with zero further drift.
                            // Bonus: this is a THIRD independent confirmation of `numsprs`
                            // @+0x8CE (read here as the inconsistent-block-count check).
  char _pad_align_scripts[2];   // +0x39F2..0x39F4, compiler alignment padding (not a real
                            // field) -- boxed in by the confirmed memset's own end and
                            // `scripts`'s own confirmed start (needs 4-byte alignment for the
                            // pointer field immediately below).
  char *scripts;                 // +0x39F4, high confidence: `load_room` (already matched)'s own
                            // pre-load cleanup does "if ([rstruc+0x39F4]!=0) { free([rstruc+
                            // 0x39F4]); [rstruc+0x39F4]=0; }" -- a plain `free()`, matching
                            // 2011's declared `char *scripts;` (`acroom.h:861`, a raw malloc'd
                            // buffer, not requiring a specialized destructor). SECOND
                            // confirmation: `load_room`'s own block-type dispatch loop (see the
                            // finding immediately below) handles `BLOCKTYPE_SCRIPT`(2) by
                            // `malloc`ing into this SAME address, `fread`ing the raw script text,
                            // NUL-terminating it, then decrypting it in place against the "Avis
                            // Durgan" key -- matching this build's old, pre-CSCOMP SeeR-era
                            // text-script format exactly.
                            //
                            // MAJOR FINDING: `load_room` doesn't read the room file directly --
                            // it dispatches on a per-block TYPE byte (`fgetc`) to one of several
                            // handler branches, matching source's own block-tagged room-file
                            // container format (`BLOCKTYPE_MAIN`=1 dispatches to
                            // `load_main_block`, `BLOCKTYPE_SCRIPT`=2/`BLOCKTYPE_COMPSCRIPT3`=7
                            // handle `scripts`/`compiled_script` as above, `BLOCKTYPE_
                            // OBJECTNAMES`=5 handles `objectnames[]` above,
                            // `BLOCKTYPE_ANIMBKGRND`=6 handles `ebscene[]`/`num_bscenes`/
                            // `bscene_anim_speed` -- see those fields' own entries). This
                            // build's dispatch loop explicitly handles ONLY types 1, 2, 3, 4, 5,
                            // 6, 7, and the `0xFF` EOF sentinel -- types 3(`COMPSCRIPT`) and
                            // 4(`COMPSCRIPT2`) both hit an explicit "Load_room: old room format.
                            // Please upgrade the room." `quit()`, and any OTHER type value
                            // (including 8=`BLOCKTYPE_PROPERTIES` and 9=`BLOCKTYPE_
                            // OBJECTSCRIPTNAMES`) falls through to a generic "LoadRoom: unknown
                            // block type %d encountered" `quit()`. This CONFIRMS ABSENT, with
                            // direct positive evidence (not just an unfound handler -- this
                            // build's code would actively CRASH if it ever encountered either
                            // block type), `objectscriptnames[MAX_INIT_SPR][MAX_SCRIPT_NAME_LEN]`
                            // and the `CustomProperties`-based `objProps[]`/`roomProps`/
                            // `hsProps[]` trio.
                            //
                            // Separately: `load_main_block`'s COMPLETE set of room-file-version
                            // gate checks -- every `cmp arg_C,N` in the entire function, listed
                            // exhaustively -- runs 3,4,5,6,7,8,9,10,11,12,13,14 with NOTHING
                            // above 14 anywhere. This means this build's COMPILED ENGINE, not
                            // just this game's own room files, never had code for room-format
                            // version 15+ at all -- the branches were never compiled in, not
                            // merely unexercised at runtime. This retroactively CONFIRMS ABSENT,
                            // with a single unifying explanation, everything gated version>=15
                            // in 2011's source: `walk_area_zoom2`/`walk_area_top`/
                            // `walk_area_bottom` (version>=18), `numLocalVars`/`localvars`
                            // (version>=19, round 7), `numRegions`/`regionLightLevel`/
                            // `regionTintLevel`/the entire `NewInteraction`-based `intrHotspot`/
                            // `intrObject`/`intrRoom`/`intrRegion` deserialization block
                            // (version>=21), `hotspotScriptNames` (version>=24), `gameId`
                            // (version>=25), and `hotspotScripts`/`objectScripts`/
                            // `regionScripts`/`roomScripts`/`deserialize_interaction_scripts`
                            // (version>=26). All CONFIRMED ABSENT from this build by the same
                            // evidence, not individually guessed. Also settles a previously-
                            // unresolved detail: the message-string-reading helper
                            // `sub_403024` (round 4) is unconditionally called with no version
                            // gate visible at its call site, consistent with it being
                            // `fgetstring_limit` (2011's version<22 path) rather than
                            // `read_string_decrypt` (version>=22) -- this build's version
                            // ceiling makes that the only path that could exist here anyway.
  ccScript *compiled_script;     // +0x39F8, high confidence: same cleanup pattern as `scripts`
                            // immediately above, but freed via a DIFFERENT helper -- matching
                            // 2011's declared `ccScript *compiled_script;` (`acroom.h:862`)
                            // needing its own specialized destructor (a `ccScript*`, not a raw
                            // buffer). RETYPED from `void*` (this round): the helper,
                            // `sub_42A4DB`, is now matched to `ccFreeScript`
                            // (`Common/cscommon.cpp:116`) -- an exact, line-for-line match:
                            // conditionally frees `globaldata`/`code`/`strings`/`fixuptypes`/
                            // `fixups` then zeroes all five, then loops `for(aa=0;
                            // aa<numimports;aa++) if(imports[aa]) free(imports[aa]);` and `for
                            // (aa=0;aa<numexports;aa++) free(exports[aa]);` (2011's own source
                            // has NO null check on the second loop either, matching this build's
                            // unconditional free exactly), then zeroes `numimports`/`numexports`
                            // and returns. This gives `ccScript`'s own struct TWO pieces of new
                            // confirmation: this build's destructor never frees `imports`/
                            // `exports`/`export_addr` themselves (only their individual
                            // elements), independently reinforcing the already-suspected drift
                            // that these are FIXED embedded `[600]` arrays here rather than
                            // 2011's separately-`malloc`'d dynamic arrays (2011's own
                            // `ccFreeScript` frees `imports`/`exports`/`export_addr` themselves
                            // right after, `cscommon.cpp:160-167` -- absent here since there's
                            // nothing to free). Also confirms 2011's trailing `numSections`/
                            // `sectionNames`/`sectionOffsets` cleanup block (`cscommon.cpp:
                            // 148-157`) is CONFIRMED ABSENT from this build's `ccScript` --
                            // `sub_42A4DB` returns immediately after the exports loop, with no
                            // third loop and no `sectionNames`-related cleanup at all. SECOND
                            // confirmation of `compiled_script` itself: `load_room`'s block-type
                            // dispatch loop handles `BLOCKTYPE_COMPSCRIPT3`(7) by assigning this
                            // SAME address the return value of the already-matched
                            // `fread_script` (`Common/CSRUN.CPP:2029`) -- matching 2011's
                            // `ccScript` deserialization exactly, and confirming this build's
                            // room files ship modern CSCOMP-compiled scripts, not the old
                            // SeeR-era text format `scripts` handles.
  int cscriptsize;               // +0x39FC, RESOLVED (this round): confirmed via
                            // `roomstruct__roomstruct` (this build's `RoomStruct` default
                            // constructor, newly matched this round -- see its own matches.json
                            // entry): "[rst+0x3880]=0x140; [rst+0x3882]=0xC8; [rst+0x39F4]=0;
                            // [rst+0x39F8]=0; [rst+0x39FC]=0;" matches 2011's own constructor
                            // "width=320; height=200; scripts=NULL; compiled_script=NULL;
                            // cscriptsize=0;" (`acroom.h:885-886`) LINE FOR LINE, in the exact
                            // same order -- five consecutive literal-value writes matching five
                            // consecutive source assignments with zero deviation, decisively
                            // resolving this position as `cscriptsize` (2011's declared `int
                            // cscriptsize;`, `acroom.h:863`, sitting immediately after
                            // `compiled_script` in source order, exactly as found here).
  int num_bscenes;               // +0x3A00, high confidence: `load_room`'s cleanup loop uses
                            // this as its bound ("for(c=1;c<[rstruc+0x3A00];c++)
                            // destroy_bitmap(ebscene[c]);"), then resets it to the literal 1
                            // afterward -- matching 2011's own constructor default
                            // ("num_bscenes=1;", `acroom.h:889`) exactly.
  int bscene_anim_speed;         // +0x3A04, high confidence: reset to the literal 5 in the same
                            // `load_room` cleanup block, matching 2011's own constructor default
                            // ("bscene_anim_speed=5;", `acroom.h:890`) exactly. 2011 declares
                            // this immediately after `num_bscenes` ("int num_bscenes,
                            // bscene_anim_speed;", `acroom.h:864`) -- zero drift.
  int bytes_per_pixel;           // +0x3A08, RESOLVED (this round), two independent confirmations:
                            // (1) `load_main_block` itself: "if (version>=12) _acroom_bpp =
                            // getw(Stream); else _acroom_bpp=1; if (_acroom_bpp<1) _acroom_bpp=1;
                            // [rst+0x3A08]=_acroom_bpp;" (immediately followed by the
                            // already-confirmed `numobj` fread) matches 2011's "if (rfh.version>=12)
                            // _acroom_bpp=getw(opty); else _acroom_bpp=1; if (_acroom_bpp<1)
                            // _acroom_bpp=1; rstruc->bytes_per_pixel=_acroom_bpp;" (`acroom.h:
                            // 1641-1649`) line for line, including the version gate, the default,
                            // AND the clamp; (2) `roomstruct__roomstruct` (this build's `RoomStruct`
                            // default constructor, newly matched this round): "[rst+0x3A0C]=0;
                            // [rst+0x3A04]=5; [rst+0x3A08]=1;" matches 2011's own constructor
                            // "num_bscenes=1; ebscene[0]=NULL; bscene_anim_speed=5;
                            // bytes_per_pixel=1;" (`acroom.h:889-890`) exactly (see `ebscene`/
                            // `bscene_anim_speed`'s own entries for the matching confirmations of
                            // their two literals in the same block).
  void *ebscene[5];              // +0x3A0C..0x3A20 (20 bytes, 5 x 4-byte pointers), the array's
                            // OWN base address is high confidence -- `load_room`'s own pre-load
                            // cleanup loop ("for(c=1;c<num_bscenes;c++) {
                            // destroy_bitmap([rstruc+c*4+0x3A0C]); [rstruc+c*4+0x3A0C]=0; }")
                            // matches 2011's declared `block ebscene[MAX_BSCENE];`
                            // (`acroom.h:866`) cleanup exactly. SECOND confirmation: `load_room`'s
                            // `BLOCKTYPE_ANIMBKGRND`(6) handler loops "for(c=1;c<num_bscenes;c++)
                            // { fpos=load_lzw(Stream,[rstruc+c*4+0x3A0C],pal); [rstruc+c*4+
                            // 0x3A0C]=recalced; }" -- matching source's own (commented-out-in-
                            // 2011, see `bpalettes` note below) "ebscene[ct]=recalced;" loop
                            // exactly. THIRD and FOURTH confirmations (also this round):
                            // `load_new_room` (already matched) contains TWO separate
                            // `for(c=0;c<num_bscenes;c++)` loops over this same array base --
                            // one converting each `ebscene[c]`'s color depth after a room
                            // change, one conditionally resizing each entry when `resolution`
                            // no longer matches the current screen multiplier -- both using the
                            // SAME confirmed `num_bscenes` bound and this SAME base address.
                            // FIFTH confirmation (this round): `RawSaveScreen`/
                            // `RawRestoreScreen`/`RawDrawImage` (all already matched) read the
                            // active background via `dword_523094[bg_frame*4]`, matching 2011's
                            // own `"abuf=thisroom.ebscene[play.bg_frame]"` (`AC.CPP:14355`)
                            // exactly -- `dword_523094` is this SAME `ebscene[0]` address
                            // (`+0x3A0C`), just accessed via IDA's own auto-generated standalone-
                            // global name in functions that don't receive `rstruc` as a parameter
                            // (unlike `load_room`), so IDA hasn't resolved it as a struct-relative
                            // access. Independently reconfirmed via decisive zero-slack
                            // arithmetic: `dword_523088`/`52308C`/`523090`/`523094` sit at
                            // consecutive +4-byte offsets, matching `num_bscenes`@+0x3A00/
                            // `bscene_anim_speed`@+0x3A04/`bytes_per_pixel`@+0x3A08/`ebscene[0]`@
                            // +0x3A0C exactly.
                            // Capacity NOT independently confirmed for THIS build
                            // though -- sized to `5` here matching 2011's own `#define
                            // MAX_BSCENE 5` (`acroom.h:803`) as the current best estimate
                            // (CORRECTING an earlier version of this comment that guessed `10`
                            // purely from this project's general smaller-capacity pattern,
                            // without having actually checked what 2011's own constant even was
                            // -- an unforced error caught immediately after `anims[]`, two fields
                            // prior, turned out to be a genuine ZERO-DRIFT match rather than the
                            // assumed drift too). CHECKED THIS ROUND, still not independently
                            // confirmed -- but now via an EXHAUSTIVE search rather than an
                            // unexplored lead: every single reference to `+0x3A0C` anywhere in the
                            // entire disassembly (6 total, including the constructor's own
                            // `ebscene[0]=NULL` write) was enumerated, and all of them either
                            // write a single fixed index (`ebscene[0]` specifically) or loop
                            // bounded by the DYNAMIC `num_bscenes` field -- never a fixed capacity
                            // literal. This exactly matches 2011's OWN idiom
                            // (`SetBackgroundFrame`/`GetBackgroundFrame`/etc. all bounds-check
                            // user input against `thisroom.num_bscenes`, the room's own declared
                            // count, NOT `MAX_BSCENE` the array capacity -- `Engine/AC.CPP:21027`
                            // etc.), so the absence of a literal-5 check isn't itself surprising --
                            // but it also means this project's usual technique (a bounds-check
                            // quit-message or malloc/array-index literal) has no site left to find
                            // it at. Capacity remains genuinely unconfirmable by this route; sized
                            // to 5 purely as 2011's own current constant, not build-specific
                            // evidence. `ebscene[0]` itself is populated separately, in
                            // `load_main_block`, via the `load_lzw`/`recalced` sequence discussed
                            // at `+0x00` above.
  // `bpalettes[MAX_BSCENE][256]` (2011's declared per-background-frame palette array,
  // `acroom.h:867`) is CONFIRMED ABSENT from this build: `load_room`'s `BLOCKTYPE_ANIMBKGRND`(6)
  // handler passes the SAME shared `rstruc+0x14` (the already-confirmed `pal[256]`) as the
  // palette argument to `load_lzw` for EVERY background-scene index, not a per-index
  // `bpalettes[ct]` address -- matching 2011's own OLDER, commented-out predecessor line still
  // sitting right next to the live code ("// fpos = load_lzw(files,rstruc->ebscene[ct],
  // rstruc->pal,fpos);", `acroom.h:2162`, superseded in 2011 by the live
  // "load_lzw(opty,rstruc->ebscene[ct],rstruc->bpalettes[ct])") -- another case, like the almp3
  // `MP3CHUNKSIZE` find, of this build matching a historical artifact still preserved in the
  // reference source's own comments. This build shares ONE palette across every background
  // frame rather than storing one per frame.
  // `localvars`/`numLocalVars` (2011's declared `InteractionVariable *localvars; int
  // numLocalVars;`, `acroom.h:868-869` -- per-room script variables, read via a version>=19
  // `getw()` gate in source) are CONFIRMED ABSENT from this build: `load_main_block`'s ENTIRE
  // body (already fully read across this struct's investigation) contains exactly TWO `getw()`
  // calls total, both already independently identified (`_acroom_bpp`'s version>=12 read, and
  // the walk-area-count override's version>=14 read) -- there is no third `getw()` call anywhere
  // for a version>=19 gate, meaning this build's compiled room-loading code never had this
  // branch at all. Independently reinforced by `unload_old_room`/`EndSkippingUntilCharStops`-
  // combined (already exhaustively read across multiple earlier GameState/RoomStatus rounds)
  // never containing the matching "for(ff=0;ff<thisroom.numLocalVars;ff++)
  // croom->interactionVariableValues[ff]=thisroom.localvars[ff].value;" copy loop either -- two
  // independent negative results agreeing with each other, the same cross-confirmation standard
  // used for every other confirmed-absent finding in this project.
  // FOUND VIA `roomstruct__roomstruct` (this build's newly-matched default constructor, closing
  // the loop on `localvars`/`numLocalVars` a THIRD independent way, and confirming three more
  // fields absent for the first time): the function's body ENDS -- a bare `retn` -- immediately
  // after `bytes_per_pixel@+0x3A08=1`. 2011's own constructor (`acroom.h:889-901`) continues past
  // the equivalent point with `numLocalVars=0; localvars=NULL; lastLoadNumHotspots=0;
  // lastLoadNumRegions=0; lastLoadNumObjects=0;` and then a `for` loop initializing
  // `walk_area_zoom2[]`/`walk_area_top[]`/`walk_area_bottom[]` (already independently confirmed
  // absent via round 8's version-ceiling argument). Since this function has now been read
  // completely, start to end, its FAILURE to contain any of those assignments is direct positive
  // evidence, not merely an unfound access site: this build's `RoomStruct` has no
  // `lastLoadNumHotspots`/`lastLoadNumObjects`/`lastLoadNumRegions` fields at all (both fields are
  // 2011-only per-room reload bookkeeping this build's ancestor layout predates), on top of the
  // now triply-confirmed absence of `localvars`/`numLocalVars` and the already-known absence of
  // the `walk_area_zoom2`/`top`/`bottom` trio. `CustomProperties hsProps[MAX_HOTSPOTS]`/`gameId`
  // (`acroom.h:872-873`) are not part of 2011's own constructor either, so this evidence doesn't
  // speak to them directly, but both are already independently confirmed absent via round 8's
  // version-ceiling argument (`CustomProperties` gated version>=21, `gameId` version>=25).
  //
  // Everything past +0x3A20 remains unexplored territory -- the struct's own total size is still
  // not established -- but every 2011-declared field known to occupy that region specifically
  // (`bpalettes`, `localvars`/`numLocalVars`, and now `lastLoadNumHotspots`/`lastLoadNumObjects`/
  // `lastLoadNumRegions`) is confirmed absent, narrowing what could still be there considerably:
  // only `ebpalShared[MAX_BSCENE]` (`acroom.h:870`, "used internally by engine atm" per 2011's own
  // comment -- plausibly a later addition itself, unconfirmed either way) and `CustomProperties
  // hsProps[MAX_HOTSPOTS]`/`gameId` (both already confirmed absent) remain as 2011-declared
  // candidates for this space at all. Every field `load_room`'s, `load_main_block`'s, and
  // `roomstruct__roomstruct`'s own read/write sequences reference has now been mapped; further
  // progress needs either a genuinely new evidence source (a different already-matched function
  // touching `rstruc`) or independently confirming `ebscene[]`'s real capacity for this build.
};

// AmbientSound (this build's version) -- FRESH SURVEY, picked as the next target after
// RoomStruct's 10-round pass exhausted every already-matched function touching `rstruc`.
// 2011 declares `AmbientSound ambient[MAX_SOUND_CHANNELS+1];` (`Common/acruntim.h:25-33,891` --
// `int channel; int x,y; int vol; int num; int maxdist;`, 24 bytes/entry) as a genuine ARRAY,
// indexed per sound channel. This build's `PlayAmbientSound` (already matched) is CONFIRMED to
// have NO SUCH ARRAY: its very first check is "if (channel!=1) quit(\"!PlayAmbientSound: channel
// must be 1\");" -- a hard-coded single-channel restriction, unlike 2011's range check against
// `MAX_SOUND_CHANNELS`. Every field write that would be `ambient[channel].FIELD` in 2011 instead
// targets a BARE SCALAR GLOBAL here -- this build never needed an indexable struct because it
// only ever supports exactly one ambient sound channel:
//   dword_4EDA68 = num       // sound number (sndnum), matching `ambient[channel].num`
//   dword_4EDA6C = maxdist   // computed as "((x>width/2)?x:(width-x))-25" in PlayAmbientSound,
//                            // matching "((x>thisroom.width/2)?x:(thisroom.width-x))-
//                            // AMBIENCE_FULL_DIST" with AMBIENCE_FULL_DIST=25 exactly (`width`
//                            // read via the already-confirmed `RoomStruct.width`, `word_522F08`)
//                            // -- reconfirmed a second, independent way by `update_ambient_
//                            // sound_vol` (below) reading it as the falloff divisor.
//   x, y                     // position, matching `ambient[channel].x/.y` -- genuinely named
//                            // globals (not this project's naming), each individually confirmed
//                            // via a DATA XREF to `update_ambient_sound_vol` below.
//   vol                      // base volume, matching `ambient[channel].vol`, same confirmation.
//   dword_4EDA58             // the loaded `SOUNDCLIP*` itself -- this build's equivalent of
//                            // `channels[chan]` for the ambient channel specifically (2011 keeps
//                            // ambient sounds in the shared `channels[]` array indexed by
//                            // `ambient[channel].channel`; this build has no such indirection,
//                            // just one dedicated pointer).
// `ambient[channel].channel` itself is CONFIRMED ABSENT as a stored field -- with `channel`
// hard-locked to `1` by the validation check, storing it back would be redundant, and no write
// site for it was found.
//
// New function match: `update_ambient_sound_vol` (`sub_4089CC`, upgraded this round from an
// unnamed medium-confidence GameState-field lead -- see its own entry) -- its body is a complete,
// exact match to source's per-channel distance-based volume falloff: "if (x==0 && y==0) use full
// volume; else { dist=sqrt((playerchar->x-x)^2+(playerchar->y-y)^2); if (dist<25) full volume;
// else full volume -= ((dist-25)*full volume)/maxdist; }" -- implemented here for the single
// hardcoded channel using the bare scalars above instead of looping over an array. Also confirms
// `GameState.sound_volume` a further independent way (already established elsewhere).
//
// DRIFT noted in passing: `PlayAmbientSound` tries MP3 first (`sub_408811` = `my_load_static_mp3`,
// see below) then falls back to WAV (`my_load_wave`, already matched) -- unlike 2011's later,
// unified `load_sound_from_path()` call.
//
// CORRECTION (immediate follow-up round): `sub_408811` is `my_load_static_mp3` (`acsound.cpp:
// 439-477`), confirmed via a COMPLETE, decisive field-offset match (vol@+0xC, mp3buffer@+0x10,
// repeat@+0x14, tune@+8, matching the real `MYSTATICMP3` member order exactly) -- not just an
// algorithm-shape guess. This REQUIRED retracting an earlier, weaker match: `sub_4083FC` (called
// from `PlayMusic`, already matched) had previously been matched to `my_load_static_mp3` too, on
// call-shape similarity alone (`pack_fopen`/`malloc`/`pack_fread`/`pack_fclose`/
// `almp3_create_mp3`) -- but its actual body stores `almp3_create_mp3`'s RAW return value
// directly into `dword_523214` (already established as `PlayMusic`'s own MP3-stream-handle
// global) with NO `MYSTATICMP3`-style wrapper object and NO `vol`/`mp3buffer`/`repeat` field
// assignments at all -- a fundamentally different shape once actually checked field-by-field.
// `sub_4083FC` is better understood as `PlayMusic`'s own dedicated, inlined MP3-stream-
// preparation helper, not the generic reusable function `my_load_static_mp3` is -- this build
// has TWO separate, near-identical loading implementations (one per caller), consistent with its
// established "no shared loading helper yet" pattern found repeatedly elsewhere. `sub_4083FC`'s
// own match entry retracted the name and left it unnamed rather than force an ill-fitting one.

// MYMIDI (2011's declared class, `Engine/acsound.cpp:916-1007`) -- FRESH SURVEY, picked as the
// next target after the SOUNDCLIP/MYWAVE/MYMP3/MYSTATICMP3 family was completed, to check whether
// this build's MIDI music support has an equivalent wrapper object. It does NOT: exactly the same
// "no wrapper object, bare scalar globals instead" architecture already found for `AmbientSound`.
// `PlayMusic` (already matched)'s own MIDI-format attempt calls Allegro's `load_midi`/`play_midi`
// (see their own matches.json entries, both newly matched this round) DIRECTLY, gluing the result
// into plain globals with no intervening `new`, vtable, or virtual dispatch of any kind:
//   dword_5231B4 = the raw `MIDI*` handle itself -- matching 2011's `MYMIDI.tune` in ROLE, just
//                  unwrapped. Checked for non-NULL as the "is a MIDI currently loaded/active" gate
//                  by `scr_StopMusic`/`IsMusicPlaying`/`GetMIDIPosition`/`SeekMIDIPosition` (all
//                  already matched, all newly given field evidence this round) -- five independent
//                  confirmation sites for the same identification.
//   dword_4BD8F8 = Allegro's own `volatile long midi_pos;` global (`allegro/midi.h:110`) -- read
//                  directly by `GetMIDIPosition`/`IsMusicPlaying`, matching source's own use of the
//                  same Allegro global inside `MYMIDI::get_pos()`/`poll()` in role.
//   dword_5231BC = set to the music number on successful MIDI load, reset to 0 on the two non-MIDI
//                  fallthrough paths in `PlayMusic` -- plausibly a "currently active MIDI music
//                  number" tracker, but WRITE-ONLY everywhere else in the disassembly (zero read
//                  sites found), so left as a documented hypothesis rather than a confirmed
//                  identity.
// Allegro's own `stop_midi()`/`destroy_midi()` (both newly matched this round, called from
// `scr_StopMusic`) match 2011's `MYMIDI::destroy()` ("stop_midi(); destroy_midi(tune);
// tune=NULL;", `acsound.cpp:942-944`) call order and role exactly, just operating on the bare
// global instead of an object's own field. `MYMIDI.lengthInSeconds` (2011's own added field, used
// only by `get_length_ms()`) has no counterpart found in this build -- consistent with the "no
// object, no per-instance state beyond what a handful of globals already cover" picture, though
// not itself independently searched for as an absence (this build may simply never call the
// equivalent of `GetMusicLength`-style APIs, if any exist here at all -- not checked this round).

// MYMOD (2011's declared class, `Engine/acsound.cpp:1030-1110`, the `JGMOD_MOD_PLAYER`-gated
// version -- matching this build's own confirmed music library, see `reversing/notes/
// third-party-library-identification.md`) -- FRESH SURVEY, an immediate follow-up to `MYMIDI`
// above. Same conclusion: NO wrapper object, bare globals instead. `PlayMusic`'s own `.mod`/`.xm`/
// `.s3m` cascade calls `load_mod` (already matched) directly, storing the result into
// `dword_5231B8` -- the bare `JGMOD*` handle, matching 2011's `MYMOD.tune` in ROLE, just
// unwrapped. Confirmed via FIVE already-matched functions all agreeing on this one global: `
// PlayMusic` (sets it, then passes it straight to `play_mod` alongside the already-confirmed
// `GameState.music_repeat`), `scr_StopMusic` (clears it, via a `is_mod_playing()`-gated
// `stop_mod()` call followed by an unconditional `destroy_mod()` -- matching 2011's `MYMOD::
// destroy()` call order exactly, `acsound.cpp:1053-1058`), and `IsMusicPlaying` (checks
// `is_mod_playing()` as one leg of its "is ANY music format active" OR-chain, alongside the
// already-established MIDI check). `is_mod_playing`/`stop_mod`/`destroy_mod` are all newly
// matched this round (no local JGMOD source tree exists in this repo to verify their exact names
// against beyond the `acsound.cpp` call-site declarations themselves -- same caveat already
// recorded on `load_mod`/`play_mod`'s own entries).

// MYOGG/MYSTATICOGG (2011's declared Ogg Vorbis classes, `Engine/acsound.cpp:494-915`) --
// CONFIRMED ABSENT, not merely unformalized. An exhaustive search for the substring "ogg" (any
// case) across BOTH the entire 2727-entry extracted string dataset AND a direct grep of the whole
// 917k-line disassembly turns up ZERO occurrences anywhere -- the strongest possible negative
// result this project's technique can produce, the same standard already used to rule out
// `apeg-1.2.1`/`dumb-0.9.2` entirely. This build's compiled engine has no Ogg Vorbis support
// whatsoever: no `MYOGG`/`MYSTATICOGG` wrapper objects (unsurprising, following `MYMIDI`/`MYMOD`'s
// own "no wrapper object" pattern), no `my_load_ogg`/`my_load_static_ogg` functions, and -- unlike
// `apeg`/`dumb`, which are absent third-party LIBRARIES this build simply doesn't link -- not even
// the underlying vorbisfile/ogg.h dependency itself appears to have been a concept yet at this
// build's 2002-07-21 link date, consistent with OGG support being a later AGS-era addition
// entirely, added sometime after MP3 support (`almp3`) had already matured. With this, ALL FOUR
// `SOUNDCLIP`-family siblings surveyed this session (`MYMIDI`/`MYMOD`/`MYOGG`/`MYSTATICOGG`) are
// accounted for -- two confirmed absent as wrapper objects (replaced by bare globals), two
// confirmed absent as a feature entirely. DATED EXACTLY (found later, via the newly-added
// `ags-archives/` resource -- see `reversing/notes/ags-archives-cross-reference.md`):
// `ags-archives/ags250/docs/CHANGES.TXT`'s "VERSION 2.5, September 2002" entry says "Added OGG
// Vorbis support for music, speech and sound effects" -- confirming this "later AGS-era addition"
// hypothesis precisely: OGG support arrived two months after Rob Blanc 1's own ~2.4b (July 2002)
// build, not merely "sometime later."

// SOUNDCLIP and its derived classes (MYWAVE/MYMP3/MYSTATICMP3) -- FRESH SURVEY. 2011's real
// `SOUNDCLIP` base class (`Common/acsound.h:22-99`) is a large, mature abstract base: 13 `int`
// fields (done/priority/soundType/vol/volAsPercentage/originalVolAsPercentage/volModifier/
// paused/panning/panningAsPercentage/xSource/ySource/maximumPossibleDistanceAway/
// directionalVolModifier), a `bool repeat`, and a `void *sourceClip`, on top of its vtable --
// roughly 0x40 bytes of base-class overhead before any derived class's own fields even start.
// This build's version is drastically smaller: evidence gathered across the `my_load_wave`/
// `my_load_mp3`/`my_load_static_mp3` matches (several rounds, now consolidated here) shows EVERY
// derived class's own first field sitting at just `+0x08` -- meaning this build's `SOUNDCLIP`
// base is only `{vtable; int done;}`, 8 bytes total. A genuine, large architectural
// simplification (not just smaller array capacities like most drift found elsewhere in this
// project) -- this build predates `SOUNDCLIP` growing volume-percentage tracking, directional/
// positional audio, and pause/resume state entirely. Represented as flat, non-inheriting structs
// below (repeating the shared vtable/done pair in each), consistent with this project's existing
// convention for C++ classes (see `FLAT_CPP_NAMES` in `extract_prototypes.py`).
struct SOUNDCLIP {
  void *vtbl;                     // +0x00, high confidence: every derived class below is
                            // allocated via `operator new` then passed through a distinct
                            // constructor call before any field gets set -- standard C++ vtable-
                            // pointer setup, inferred from the consistent pattern across all
                            // three derived classes rather than directly observed (constructor
                            // bodies themselves not disassembled this round).
  int done;                        // +0x04, high confidence: `my_load_wave`/`my_load_mp3`
                            // (already matched) both explicitly zero this field right after
                            // construction ("[thiswave+4]=0"/"[thistune+4]=0"), matching 2011's
                            // `SOUNDCLIP()` constructor's own "done=0;" (`acsound.h:85`) -- the
                            // one base-class field this build's simplified version keeps.
};                          // Total confirmed size: 8 bytes (vs. 2011's ~0x40).

struct MYWAVE {
  void *vtbl;                     // +0x00, see `SOUNDCLIP.vtbl` above.
  int done;                        // +0x04, see `SOUNDCLIP.done` above.
  void *wave;                       // +0x08, high confidence: `my_load_wave` (`sub_408556`,
                            // already matched) stores the just-loaded Allegro `SAMPLE*` (from
                            // `load_sample`, already matched) here immediately after
                            // construction -- matching 2011's `SAMPLE *wave;` (`acsound.cpp:52`,
                            // `MYWAVE`'s own declared FIRST field) exactly in both role and
                            // position, zero drift.
  int voice;                        // +0x0C, high confidence: the Allegro voice handle returned
                            // by `play_sample` (already matched) is stored here directly --
                            // matching 2011's declared `int voice;` (`acsound.cpp:53`,
                            // `MYWAVE`'s own declared SECOND field) exactly in position, though
                            // its ROLE differs: 2011's `voice` gets set lazily inside `play()`
                            // (called separately, after construction), while this build's
                            // `my_load_wave` calls `play_sample` immediately at LOAD time and
                            // stores the handle here right away. ARCHITECTURAL FINDING /
                            // CONFIRMED ABSENT: 2011's trailing `int firstTime; int repeat;`
                            // (`acsound.cpp:54-55`) have no room here -- the struct's own
                            // confirmed total size (16 bytes) ends immediately after `voice`,
                            // proving this build's `MYWAVE` plays its sample eagerly at load
                            // time rather than storing parameters for a later lazy `play()`.
};                          // Total confirmed size: 16 bytes (0x10), matching the `operator
                            // new(0x10)` call exactly with zero slack.

struct MYMP3 {
  void *vtbl;                     // +0x00, see `SOUNDCLIP.vtbl` above.
  int done;                        // +0x04, see `SOUNDCLIP.done` above.
  void *stream;                     // +0x08, high confidence: `my_load_mp3` (`sub_408623`,
                            // already matched) stores `almp3_create_mp3stream`'s result here
                            // (`sub_47ED10`, this round's new match -- see its own entry) --
                            // matching 2011's `thistune->stream=almp3_create_mp3stream(...);`
                            // (`acsound.cpp:320`) exactly in both role and position (2011's
                            // `MYMP3` also declares `stream` as its own first member,
                            // `ALMP3_MP3STREAM *stream;`).
  void *in;                         // +0x0C, high confidence: stores the `PACKFILE*` from
                            // `pack_fopen` (already matched) -- matching 2011's `PACKFILE *in;`
                            // (`acsound.cpp:180`, `MYMP3`'s own declared SECOND field) exactly
                            // in both role and position, zero drift.
  void *buffer;                     // +0x10, high confidence: stores the `malloc`'d
                            // decompression buffer after `pack_fread` fills it -- matching
                            // 2011's `char *buffer;` (`acsound.cpp:182`, `MYMP3`'s own declared
                            // FOURTH field) in role, but sitting with ZERO gap immediately after
                            // `in`@`+0x0C` (`+0x10`) -- CONFIRMING 2011's intervening `long
                            // filesize;` (`acsound.cpp:181`, `MYMP3`'s own declared THIRD field)
                            // is CONFIRMED ABSENT here, not merely unfound: there is no room for
                            // it between two independently-confirmed neighbors.
  int chunksize;                    // +0x14, high confidence: set to the literal `0x186A0`
                            // (100000) -- matching `MP3CHUNKSIZE`'s OLD, commented-out value in
                            // 2011's own source (already established, several rounds ago, as a
                            // genuine drift point matching a historical artifact preserved in
                            // the reference source's comments) -- matching 2011's `int
                            // chunksize;` (`acsound.cpp:183`, `MYMP3`'s own declared FIFTH/LAST
                            // field) in role and position, zero drift.
};                          // Total confirmed size: 24 bytes (0x18), matching the `operator
                            // new(0x18)` call exactly with zero slack -- `stream`/`in`/
                            // `chunksize` match 2011's own declared `MYMP3` member positions
                            // with zero drift, `buffer` shifts 4 bytes earlier due to the
                            // confirmed-absent `filesize`, and the `SOUNDCLIP` base portion
                            // itself is drastically smaller (see the struct's own entry above)
                            // -- a clean example of this project's usual pattern (later fields
                            // absent) layered on top of its less usual one (base class shrunk).

struct MYSTATICMP3 {
  void *vtbl;                     // +0x00, see `SOUNDCLIP.vtbl` above.
  int done;                        // +0x04, see `SOUNDCLIP.done` above.
  void *tune;                       // +0x08, high confidence: `my_load_static_mp3`
                            // (`sub_408811`, this round's new match) stores `almp3_create_mp3`'s
                            // result here -- matching 2011's `ALMP3_MP3 *tune;`
                            // (`acsound.cpp:335`, `MYSTATICMP3`'s own declared FIRST field, set
                            // via `thismp3->tune=almp3_create_mp3(...);` at `acsound.cpp:465`)
                            // exactly in both role and position, zero drift.
  int vol;                          // +0x0C, high confidence: set directly from the function's
                            // own `voll` parameter -- matching 2011's `thismp3->vol=voll;`
                            // (`acsound.cpp:462`) in role, but NOT position: 2011's `MYSTATICMP3`
                            // doesn't declare its own `vol` field at all (it's inherited from the
                            // much larger `SOUNDCLIP` base, `acsound.h:27`) -- this build's
                            // drastically smaller `SOUNDCLIP` base (see that struct's own entry
                            // above) doesn't provide one, so this derived class declares its own
                            // local `vol` to compensate, landing right after `tune` instead.
  void *mp3buffer;                   // +0x10, high confidence: initialized to `NULL` immediately
                            // after construction, then set to the real buffer pointer after a
                            // successful `almp3_create_mp3` call -- matching 2011's `char
                            // *mp3buffer;` (`acsound.cpp:336`, `MYSTATICMP3`'s own declared
                            // SECOND field) in role, shifted 4 bytes later than 2011's position
                            // by the locally-reclaimed `vol` field immediately before it.
  char repeat;                      // +0x14, high confidence: set directly from the function's
                            // own `loop` parameter (read as a single byte) -- matching 2011's
                            // `thismp3->repeat=loop;` (`acsound.cpp:464`) in role, but -- like
                            // `vol` above -- NOT position: 2011's `bool repeat` is inherited from
                            // `SOUNDCLIP` (`acsound.h:37`), not declared locally in
                            // `MYSTATICMP3` at all. This build's version reclaims it as its own
                            // local field too, landing last, after `mp3buffer`.
};                          // Total confirmed size: 24 bytes (0x18), matching the `operator
                            // new(0x18)` call exactly with zero slack -- `tune`/`vol`/
                            // `mp3buffer`/`repeat` all matching 2011's own declared
                            // `MYSTATICMP3` member order exactly (only the `SOUNDCLIP` base
                            // portion drifts, not this derived class's own fields -- the same
                            // pattern as `MYMP3` above).

// SpriteListEntry (this build's own ancestor version) -- found via a self-identifying error
// string sweep: `sub_4106EF` carries its own name in its overflow-quit message,
// "ad_to_sprite_list: roo many sprite added" (a genuine typo in the original 2002 source,
// "roo" for "too" -- preserved verbatim since it's the function's own compiled string, not a
// transcription error). Confirmed as `add_to_sprite_list` (`Engine/AC.CPP:7441-7470`, the
// "intermediate list used to order objects and characters by their baselines before everything
// is added to the Thing To Draw List") via a decisive combination of role AND caller: both of
// its two call sites are inside `prepare_characters_for_drawing` (already matched), one for
// drawing a room object (passing `RoomObject.transparent`@+0x08, already confirmed, as the
// 5th argument) and one for a character sprite -- matching 2011's own two call sites into
// `add_to_sprite_list` from the SAME function almost exactly in shape. A companion function,
// `sub_4106E0` ("mov dword_5231DC,0; retn"), matches 2011's `clear_sprite_list()` ("void
// clear_sprite_list() { sprlistsize=0; }", `AC.CPP:7438-7440`) verbatim -- both newly matched
// this round. `dword_5231DC` is `sprlistsize`, confirmed via the exact same global driving both
// functions' counter logic.
struct SpriteListEntry {
  void *bmp;                  // +0x00, high confidence: the function's 1st argument (the
                            // bitmap/SpriteCache-derived pointer passed by both call sites),
                            // matching 2011's own leading `IDriverDependantBitmap *bmp` field
                            // (`acsound.h`-style hardware-bitmap wrapper this build predates --
                            // here just a raw `block`/`BITMAP*`, consistent with this build's
                            // established lack of the hardware-acceleration abstraction layer
                            // found absent elsewhere, e.g. `CharacterInfo.actx`/`.acty`).
  int baseline;                // +0x04, high confidence: the function's 4th argument
                            // (`var_34` at both call sites, passed unchanged/unadjusted,
                            // matching 2011's `baseline` role and the sort-by-baseline purpose
                            // this whole list exists for).
  int x;                       // +0x08, high confidence: the function's 2nd argument (`var_10`
                            // at both call sites). RESOLVED (immediate follow-up, same
                            // investigation): `var_10`/`var_28` are computed ONCE, earlier in
                            // `prepare_characters_for_drawing`'s loop body, already WITH
                            // `offsetx`/`offsety` subtracted ("eax=obj.x*mult_x; eax-=offsetx;
                            // var_10=eax" -- converting room-space to screen-space up front).
                            // The sibling `sub_410631` call (taken only when
                            // `RoomObject.flags&OBJF_NOWALKBEHINDS`(2) is CLEAR, i.e. the
                            // walk-behind-aware sort path, already documented on `flags` above)
                            // ADDS `offsetx`/`offsety` back before its own call specifically
                            // because IT needs the original room-space coordinates for
                            // walk-behind-occlusion purposes, while `add_to_sprite_list` wants
                            // the already-converted screen-space values used everywhere else in
                            // the sprite list -- ordinary coordinate-space bookkeeping between
                            // two consumers with different needs, not an unexplained asymmetry.
  int y;                       // +0x0C, high confidence: the function's 3rd argument (`var_28`),
                            // same evidence pattern as `x` above.
  int transparent;             // +0x10, high confidence: the function's 5th argument -- at one
                            // call site this is literally `RoomObject.transparent`@+0x08
                            // (already confirmed), read via `[dword_4E45C8+ecx*0x20+8]` and
                            // passed straight through, an unusually direct field-to-field link
                            // between two independently-confirmed structs.
};                          // Total confirmed size 0x14 (20 bytes), matching the `*0x14` stride
                            // used to index all five backing globals (`dword_4DC870`/`874`/
                            // `878`/`87C`/`880`) with zero slack -- IDA auto-named each field's
                            // own computed address as an unrelated standalone global, the same
                            // "doesn't recognize the struct" pattern seen repeatedly elsewhere
                            // in this project (`ebscene[]`/`dword_523094`, `messages[500]`/
                            // `dword_51CB50`). DRIFT: 2011's own `SpriteListEntry` (`AC.CPP:
                            // 683-686`) additionally declares `bool hasAlphaChannel` and `bool
                            // takesPriorityIfEqual` -- both CONFIRMED ABSENT here (the function's
                            // own body only ever writes these 5 fields, nothing else, and takes
                            // only 5 arguments where 2011's takes 7) -- this build predates both
                            // the alpha-channel sprite flag and the walk-behind-priority-sorting
                            // features entirely. Capacity: this build's own overflow check fires
                            // at count `>=0x27`(39), roughly half of 2011's `MAX_SPRITES_ON_
                            // SCREEN=76` (`AC.CPP:685`) -- the familiar "later capacity increase"
                            // pattern found throughout this project, here for the very first time
                            // applied to a runtime PER-FRAME list rather than a fixed save-data
                            // array.
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
              "MoveList, ViewFrame272, ViewStruct272, RoomObject, AnimationStruct, "
              "FullAnimation, RoomStatus, WordsDictionary, GameState, ScreenOverlay, "
              "RoomStruct, SOUNDCLIP, MYWAVE, MYMP3, MYSTATICMP3, sprstruc, "
              "PolyPoints).")
    else:
        print(f"parse_decls reported {err_count} error(s) -- check the declarations above.")


if __name__ == "__main__":
    main()
