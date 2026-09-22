/* ags/dialog_run.h's own implementation. See that header for the
 * complete opcode table and evidence.
 */
#include "ags/dialog_run.h"
#include "ags/interp.h"
#include "ags/interaction.h"
#include "ags/audio.h"
#include "ags/inventory.h"
#include "ags/room_loader.h"
#include "ags/stub.h"

#include <stdio.h>

static short read_s16(const unsigned char *p)
{
    return (short)((unsigned int)p[0] | ((unsigned int)p[1] << 8));
}

/* DCMD_ADDINV -- now just the shared, real ags_add_inventory
 * (ags/inventory.h, promoted out of this file once M11's own
 * inventory slice built it as a proper shared module -- see that
 * header for the full evidence). Operates on chars[0] as a stand-in
 * for "playerchar" (no separate playercharacter-index lookup is
 * threaded through this context -- a real limitation for a game
 * whose player character isn't index 0, documented rather than
 * silently assumed correct for every game). */
static void add_inventory(struct AgsDialogRunContext *ctx, struct GameState *play, int inum)
{
    if (ctx->numcharacters <= 0) {
        return;
    }
    ags_add_inventory(&ctx->chars[0], play, inum);
}

/* DCMD_NEWROOM/DCMD_RUNTEXTSCRIPT's own NewRoom(nrnum) call -- a
 * SCOPED real reload (ags_load_room, M5) only. See this header's own
 * file-level comment: NewRoom's full player-repositioning/fade/
 * RoomStatus-persistence machinery is a separate subsystem, not
 * ported here. */
static void dialog_new_room(struct AgsDialogRunContext *ctx, int nrnum)
{
    char roomfile[32];
    if (!ctx->rst) {
        AGS_STUB_VOID(); /* no RoomStruct* supplied -- caller doesn't want a real room switch */
        return;
    }
    sprintf(roomfile, "room%d.crm", nrnum);
    ags_load_room(roomfile, ctx->rst); /* real, but scoped -- see the header comment above */
}

int ags_run_dialog_script(struct AgsDialogRunContext *ctx, struct DialogTopic *dtpp,
                           struct GameState *play, int offset)
{
    const unsigned char *code;

    if (offset < 0) {
        return AGS_DIALOG_RETURN; /* source's own leading "if(offse<0) return -1;" */
    }
    code = dtpp->optionscripts + offset;

    for (;;) {
        unsigned char opcode = code[0];
        const unsigned char *operand = code + 1;

        switch (opcode) {
        case 1: { /* DCMD_SAY */
            short charid = read_s16(operand);
            short msgid = read_s16(operand + 2);
            const char *text = (msgid >= 0 && msgid < ctx->numdlgmessage) ? ctx->dlgmessages[msgid] : "";
            if (charid == 999) {
                ags_display_text_box(320, 200, NULL, text); /* "narrator" sentinel */
            } else if (charid >= 0 && charid < ctx->numcharacters) {
                ags_display_text_box(320, 200, ctx->chars[charid].name, text);
            } else {
                ags_display_text_box(320, 200, NULL, text);
            }
            code += 5;
            break;
        }
        case 2: { /* DCMD_OPTOFF */
            short idx = read_s16(operand);
            if (idx >= 0 && idx < 15) {
                dtpp->optionflags[idx] &= ~1;
            }
            code += 3;
            break;
        }
        case 3: { /* DCMD_OPTON */
            short idx = read_s16(operand);
            if (idx >= 0 && idx < 15 && !(dtpp->optionflags[idx] & 2)) {
                dtpp->optionflags[idx] |= 1;
            }
            code += 3;
            break;
        }
        case 4: /* DCMD_RETURN */
            return AGS_DIALOG_RETURN;
        case 5: /* DCMD_STOPDIALOG */
            return AGS_DIALOG_STOP;
        case 6: { /* DCMD_OPTOFFFOREVER */
            short idx = read_s16(operand);
            if (idx >= 0 && idx < 15) {
                dtpp->optionflags[idx] = (dtpp->optionflags[idx] & ~1) | 2;
            }
            code += 3;
            break;
        }
        case 7: { /* DCMD_RUNTEXTSCRIPT (run_dialog_request fused in) */
            short param = read_s16(operand);
            long argval = param;
            int rc = AGS_CC_RUN_FUNC_NOT_FOUND;

            play->stop_dialog_at_end = 1; /* DIALOG_RUNNING */
            if (ctx->gameinst) {
                rc = ags_cc_call_instance(ctx->gameinst, "dialog_request", 1, &argval, NULL);
            }
            if (rc == AGS_CC_RUN_FUNC_NOT_FOUND) {
                /* Real behavior for an optional hook function the
                 * game script never defined -- not an error. */
                AGS_STUB_VOID();
            }
            if (play->stop_dialog_at_end == 2) {
                play->stop_dialog_at_end = 0;
                return AGS_DIALOG_STOP;
            }
            if (play->stop_dialog_at_end >= 100) {
                int nrnum = play->stop_dialog_at_end - 100;
                play->stop_dialog_at_end = 0;
                dialog_new_room(ctx, nrnum);
                return AGS_DIALOG_STOP;
            }
            play->stop_dialog_at_end = 0;
            code += 3;
            break;
        }
        case 8: { /* DCMD_GOTODIALOG */
            short dlgnum = read_s16(operand);
            return dlgnum;
        }
        case 9: { /* DCMD_PLAYSOUND */
            short soundnum = read_s16(operand);
            ags_play_sound(soundnum);
            code += 3;
            break;
        }
        case 0xA: { /* DCMD_ADDINV */
            short itemnum = read_s16(operand);
            add_inventory(ctx, play, itemnum);
            code += 3;
            break;
        }
        case 0xB: { /* DCMD_SETSPCHVIEW */
            short charid = read_s16(operand);
            short vw = read_s16(operand + 2);
            if (charid >= 0 && charid < ctx->numcharacters) {
                ctx->chars[charid].talkview = vw - 1;
            }
            code += 5;
            break;
        }
        case 0xC: { /* DCMD_NEWROOM */
            short roomnum = read_s16(operand);
            dialog_new_room(ctx, roomnum);
            return AGS_DIALOG_STOP;
        }
        case 0xFF: /* DCMD_ENDSCRIPT */
            return AGS_DIALOG_RETURN;
        default:
            /* source's own quit("unknown dialog command") -- this
             * build's own compiler never emits anything outside this
             * table, so real game data never reaches here. */
            AGS_STUB_VOID();
            return AGS_DIALOG_RETURN;
        }
    }
}
