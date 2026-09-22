/* M11 test (see src/PLAN.md): "the long tail" -- dialog system slice.
 * Walks the full M2/M3/M11 loader chain (through messages[500]/
 * DialogTopic[]/dlgmessage[], same as test_gui.c) plus M4's own real
 * script interpreter (to give DCMD_RUNTEXTSCRIPT's own "dialog_request"
 * hook lookup a real ccInstance* to check against), then actually RUNS
 * Rob Blanc 1's own real compiled dialog-script bytecode through
 * ags_run_dialog_script (ags/dialog_run.h) -- first each topic's own
 * startupentrypoint, then (a minimal stand-in for do_conversation's own
 * options menu, which this milestone doesn't render) the first
 * currently-on option's entrypoint, following DCMD_GOTODIALOG chains a
 * few hops deep.
 *
 * Usage:
 *   test_dialog.exe <path to rb.exe>
 */
#include "ags/clib.h"
#include "ags/loader.h"
#include "ags/script_loader.h"
#include "ags/interp.h"
#include "ags/dialog_run.h"
#include "ags/gfx.h"
#include "ags/stub.h"

#include <allegro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void run_topic(struct AgsDialogRunContext *ctx, struct GameState *play, int dlgnum)
{
    int hops;
    int cur = dlgnum;

    for (hops = 0; hops < 5 && cur >= 0; hops++) {
        struct DialogTopic *dtpp = &ctx->dlgtopics[cur];
        int rc;
        int chosen;

        printf("\n--- running dialog %d's startupentrypoint (offset %d) ---\n",
               cur, dtpp->startupentrypoint);
        rc = ags_run_dialog_script(ctx, dtpp, play, dtpp->startupentrypoint);
        printf("--- startup run returned %d ---\n", rc);
        if (rc == AGS_DIALOG_STOP) {
            printf("dialog %d: STOPDIALOG from its own startup script\n", cur);
            return;
        }
        if (rc >= 0) {
            cur = rc; /* GOTODIALOG from the startup script itself */
            continue;
        }

        /* Minimal stand-in for do_conversation's own options menu
         * (not rendered this milestone): pick the first option whose
         * optionflags[]&1 (DFLG_ON) is set. */
        chosen = -1;
        {
            int i;
            for (i = 0; i < dtpp->numoptions && i < 15; i++) {
                if (dtpp->optionflags[i] & 1) {
                    chosen = i;
                    break;
                }
            }
        }
        if (chosen < 0) {
            printf("dialog %d: no options currently on -- conversation ends\n", cur);
            return;
        }
        printf("choosing option %d: \"%s\"\n", chosen, dtpp->optionnames[chosen]);
        rc = ags_run_dialog_script(ctx, dtpp, play, dtpp->entrypoints[chosen]);
        printf("--- option %d run returned %d ---\n", chosen, rc);
        if (rc == AGS_DIALOG_STOP) {
            printf("dialog %d: STOPDIALOG after option %d\n", cur, chosen);
            return;
        }
        if (rc >= 0) {
            cur = rc;
            continue;
        }
        /* AGS_DIALOG_RETURN: real do_conversation would redisplay the
         * options menu for the SAME topic -- this stand-in just stops,
         * since there's no menu here to redisplay. */
        printf("dialog %d: option %d returned to the options menu (not redisplayed "
               "by this milestone's own minimal driver)\n", cur, chosen);
        return;
    }
}

int main(int argc, char **argv)
{
    struct GameSetupStructBase game;
    struct GameState play;
    struct AgsGameFileHeader hdr;
    struct AgsScriptBlockInfo script_info;
    struct CharacterInfo *chars = NULL;
    struct DialogTopic *dialogs = NULL;
    char **dlgmessages = NULL;
    struct AgsDialogRunContext ctx;
    struct ccScript *scri = NULL;
    struct ccInstance *inst = NULL;
    enum AgsScriptLoadError script_err;
    FILE *f;
    int rc;
    int i;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <path to rb.exe>\n", argv[0]);
        return 1;
    }

    /* ags_display_text_box (ags/interaction.h) draws onto Allegro's
     * own `screen` global -- needs a real graphics mode set up first,
     * exactly like every other visual milestone test (M6+), even
     * though this test's own acceptance check is about the bytecode
     * interpreter, not the picture. */
    if (allegro_init() != 0) {
        fprintf(stderr, "allegro_init failed\n");
        return 1;
    }
    if (ags_gfx_init_windowed(320, 200, 8) != 0) {
        fprintf(stderr, "ags_gfx_init_windowed failed: %s\n", allegro_error);
        allegro_exit();
        return 1;
    }

    if (ags_csetlib(argv[1]) != 0) {
        fprintf(stderr, "ags_csetlib failed\n");
        allegro_exit();
        return 1;
    }

    f = ags_clib_fopen("ac2game.dta", "rb");
    if (!f) {
        fprintf(stderr, "could not open ac2game.dta\n");
        return 1;
    }

#define CHECK(call) \
    do { \
        rc = (call); \
        if (rc != 0) { \
            fprintf(stderr, #call " failed: %d\n", rc); \
            fclose(f); \
            return 1; \
        } \
    } while (0)

    CHECK(ags_load_game_file_header(f, &hdr));
    CHECK(ags_load_gamesetup(f, &game));
    CHECK(ags_skip_words_dictionary(f, &game));
    CHECK(ags_skip_unidentified_block(f));

    /* M4's real script decode (not M3's skip) -- gives
     * DCMD_RUNTEXTSCRIPT a real ccInstance* to look "dialog_request"
     * up against. */
    scri = ags_cc_read_script(f, &script_err);
    if (!scri) {
        fprintf(stderr, "ags_cc_read_script failed: %d\n", (int)script_err);
        fclose(f);
        return 1;
    }
    CHECK(ags_skip_views(f, &game));
    CHECK(ags_skip_unidentified_block2(f));

    chars = (struct CharacterInfo *)malloc((size_t)game.numcharacters * sizeof(struct CharacterInfo));
    if (!chars) {
        fprintf(stderr, "out of memory (chars)\n");
        fclose(f);
        return 1;
    }
    CHECK(ags_load_characters(f, &game, chars));
    CHECK(ags_load_messages(f, &game));

    printf("numdialog=%d numdlgmessage=%d\n", game.numdialog, game.numdlgmessage);
    rc = ags_load_dialog_topics(f, game.numdialog, &dialogs);
    if (rc != 0) {
        fprintf(stderr, "ags_load_dialog_topics failed: %d\n", rc);
        fclose(f);
        return 1;
    }
    rc = ags_load_dlgmessages(f, game.numdlgmessage, &dlgmessages);
    fclose(f);
    if (rc != 0) {
        fprintf(stderr, "ags_load_dlgmessages failed: %d\n", rc);
        return 1;
    }

    printf("\ndlgmessages:\n");
    for (i = 0; i < game.numdlgmessage; i++) {
        printf("  [%d] \"%s\"\n", i, dlgmessages[i]);
    }

    for (i = 0; i < game.numdialog; i++) {
        int j;
        printf("\ndialog[%d]: numoptions=%d startupentrypoint=%d codesize=%d\n",
               i, dialogs[i].numoptions, dialogs[i].startupentrypoint, dialogs[i].codesize);
        for (j = 0; j < dialogs[i].numoptions && j < 15; j++) {
            printf("  option[%d]: \"%s\" flags=0x%x entrypoint=%d\n",
                   j, dialogs[i].optionnames[j], dialogs[i].optionflags[j], dialogs[i].entrypoints[j]);
        }
    }

    if (game.numdialog <= 0) {
        fprintf(stderr, "FAIL: this game's real data has no dialogs to run -- "
                        "test cannot demonstrate the interpreter against real data\n");
        return 1;
    }

    inst = ags_cc_create_instance(scri);
    if (!inst) {
        fprintf(stderr, "FAIL: ags_cc_create_instance returned NULL\n");
        return 1;
    }

    memset(&play, 0, sizeof(play));

    ctx.dlgmessages = dlgmessages;
    ctx.numdlgmessage = game.numdlgmessage;
    ctx.chars = chars;
    ctx.numcharacters = game.numcharacters;
    ctx.gameinst = inst;
    ctx.rst = NULL; /* no room switching demonstrated by this test */
    ctx.dlgtopics = dialogs;
    ctx.numdialogs = game.numdialog;

    for (i = 0; i < game.numdialog; i++) {
        run_topic(&ctx, &play, i);
    }

    ags_stub_dump_summary();

    allegro_exit();

    printf("\nM11 ACCEPTANCE CHECK OK (dialog system slice): real DCMD_* bytecode "
           "from this game's own compiled dialog topics was decoded and executed "
           "through the real run_dialog_script interpreter\n");
    return 0;
}
END_OF_MAIN()
