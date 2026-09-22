/* ags/dialog_run.h -- M11 ("the long tail" / dialog system, see
 * src/PLAN.md): a real, statement-by-statement port of
 * run_dialog_script (rob_blanc_1.asm, proc bounds 0x41D49B-0x41D7F8),
 * read directly from the disassembly since this build's own
 * dialog-script byte-code interpreter has NO 2011 source counterpart
 * at all to check against -- 2011 keeps the DCMD_* opcode CONSTANTS
 * declared (Common/acroom.h:2653-2669) purely for its own dead,
 * commented-out predecessor, but its real run_dialog_script compiles
 * dialog topics into actual script bytecode and calls it through
 * run_text_script_iparam instead (the same "declared but dead by
 * 2011, still fully live here" pattern already found for EventBlock/
 * AnimationStruct -- see ags/interaction.h's own file-level comment).
 *
 * COMPLETE, exhaustively-confirmed opcode table (matches.json's own
 * `run_dialog_script` entry: "any other opcode byte hits an explicit
 * 'unknown dialog command' quit(), proving the switch is EXHAUSTIVE"):
 *   1  = DCMD_SAY          [2-byte charID][2-byte msgID] -- charID==999
 *        ("narrator") displays dlgmessages[msgID] via Display(); any
 *        other charID is a character line via GetTranslation+
 *        _displayspeech. This port's own DisplayMessage/_displayspeech
 *        stand-in is ags_display_text_box (ags/interaction.h, shared
 *        with DisplayMessage) -- see this header's own note on that
 *        simplification.
 *   2  = DCMD_OPTOFF       [2-byte idx] -- optionflags[idx] &= ~1 (DFLG_ON)
 *   3  = DCMD_OPTON        [2-byte idx] -- optionflags[idx] |= 1 UNLESS
 *        DFLG_OFFPERM(2) is already set
 *   4  = DCMD_RETURN       -- returns AGS_DIALOG_RETURN(-1)
 *   5  = DCMD_STOPDIALOG   -- returns AGS_DIALOG_STOP(-2)
 *   6  = DCMD_OPTOFFFOREVER [2-byte idx] -- optionflags[idx] =
 *        (optionflags[idx]&~1)|2 (clear ON, set OFFPERM permanently)
 *   7  = DCMD_RUNTEXTSCRIPT [2-byte param] -- run_dialog_request's own
 *        ENTIRE body, fused into this one opcode case (matches.json's
 *        own correction -- run_dialog_request has NO separate identity
 *        in this build at all): sets play->stop_dialog_at_end=1
 *        (DIALOG_RUNNING), calls the game script's own optional
 *        "dialog_request(param)" export if it exists (a benign no-op,
 *        matched via ags_cc_call_instance's own AGS_CC_RUN_FUNC_NOT_
 *        FOUND, if it doesn't -- exactly this build's own real
 *        behavior for an optional hook function no game is required
 *        to define), then reacts to whatever that function may have
 *        set stop_dialog_at_end to: ==2 => AGS_DIALOG_STOP, >=100 =>
 *        NewRoom(value-100) then AGS_DIALOG_STOP, otherwise continue.
 *   8  = DCMD_GOTODIALOG   [2-byte dlgnum] -- returns dlgnum directly
 *        (the caller/do_conversation-equivalent driver is expected to
 *        switch to dialogs[dlgnum] and start it from its own
 *        startupentrypoint)
 *   9  = DCMD_PLAYSOUND    [2-byte soundnum] -- calls the already-real
 *        ags_play_sound (ags/audio.h, M10)
 *   0xA = DCMD_ADDINV      [2-byte itemnum] -- add_inventory's own
 *        real algorithm (matches.json: validate [0,100), increment
 *        CharacterInfo.inv[itemnum] on the player, append to
 *        play_invorder[]/inv_numorder if not already present) --
 *        minus its own trailing run_on_event(7,inum) hook call (no
 *        event-hook subsystem exists yet)
 *   0xB = DCMD_SETSPCHVIEW [2-byte charID][2-byte view] -- writes
 *        chars[charID].talkview = view-1 directly
 *   0xC = DCMD_NEWROOM     [2-byte roomnum] -- returns AGS_DIALOG_STOP
 *        after a SCOPED real room reload (ags_load_room("room%d.crm"),
 *        M5) -- NewRoom's own full player-repositioning/fade/
 *        RoomStatus-persistence machinery is NOT ported (a whole
 *        separate subsystem, out of scope for this dialog-focused
 *        slice; same "cite the real scope decision" convention as
 *        M8/M9's own scoping notes)
 *   0xFF = DCMD_ENDSCRIPT  -- returns AGS_DIALOG_RETURN(-1)
 *   anything else -- source's own quit("unknown dialog command");
 *        this port logs via AGS_STUB_VOID() and returns
 *        AGS_DIALOG_RETURN instead of aborting the whole process
 *        (this build's own bytecode compiler never emits an opcode
 *        outside this table, so real game data never reaches this
 *        branch -- purely a safety net).
 * CONFIRMED ABSENT (2011-only later additions, dated via ags-archives/
 * -- see reversing/notes/ags-archives-cross-reference.md): DCMD_
 * SETGLOBALINT(13, AGS 2.5), DCMD_GIVESCORE(14), DCMD_GOTOPREVIOUS(15),
 * DCMD_LOSEINV(16, both AGS 2.56) -- all strictly after this build's
 * own ~2.4b/July-2002 pin.
 */
#ifndef AGS_DIALOG_RUN_H
#define AGS_DIALOG_RUN_H

#include "ags/dialog.h"
#include "ags/gamestate.h"
#include "ags/character.h"
#include "ags/room.h"

enum AgsDialogRunResult {
    AGS_DIALOG_RETURN = -1, /* DCMD_RETURN/DCMD_ENDSCRIPT -- "fall through", e.g. redisplay the options menu */
    AGS_DIALOG_STOP = -2    /* DCMD_STOPDIALOG, or a NEWROOM-redirected case -- end the whole conversation */
    /* any value >= 0 is DCMD_GOTODIALOG's own target dialog topic number */
};

/* Bundles every piece of engine state run_dialog_script's own real
 * opcodes need to touch, deliberately no bigger than that (same
 * convention as ags/interaction.h's own AgsGameContext). `gameinst`/
 * `rst` may be NULL -- see DCMD_RUNTEXTSCRIPT/DCMD_NEWROOM's own
 * notes above for what happens then (both real, documented
 * fallbacks, not silent gaps). */
struct AgsDialogRunContext {
    char **dlgmessages;
    int numdlgmessage;
    struct CharacterInfo *chars;
    int numcharacters;
    struct ccInstance *gameinst;
    struct RoomStruct *rst;
    /* Not touched by ags_run_dialog_script itself (DCMD_GOTODIALOG
     * only ever returns a topic NUMBER, matching source's own
     * "return dlgnum;" exactly) -- carried here purely so a
     * do_conversation-equivalent caller has dialogs[]/its count on
     * hand to switch topics with, without a separate parameter. */
    struct DialogTopic *dlgtopics;
    int numdialogs;
};

/* run_dialog_script(DialogTopic *dtpp, int offse) -- see this
 * header's own file-level comment for the complete opcode table.
 * `offset<0` returns AGS_DIALOG_RETURN immediately without touching
 * `dtpp` at all, matching source's own leading "if(offse<0) return
 * -1;" exactly (used by a do_conversation-equivalent caller for an
 * option whose entrypoint is -1, i.e. "does nothing when chosen"). */
int ags_run_dialog_script(struct AgsDialogRunContext *ctx, struct DialogTopic *dtpp,
                           struct GameState *play, int offset);

#endif /* AGS_DIALOG_RUN_H */
