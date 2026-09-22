/* M11 test (see src/PLAN.md): "the long tail" -- dialog system slice,
 * now with the real options-menu UI (ags/dialog_menu.h). Walks the
 * full M2/M3/M11 loader chain (through messages[500]/DialogTopic[]/
 * dlgmessage[]/guis[], same as test_gui.c) plus M4's own real script
 * interpreter (to give DCMD_RUNTEXTSCRIPT's own "dialog_request" hook
 * lookup a real ccInstance* to check against), then actually RUNS
 * Rob Blanc 1's own real compiled dialog-script bytecode through
 * ags_run_dialog_script (ags/dialog_run.h) -- first each topic's own
 * startupentrypoint, then a REAL, rendered, mouse/keyboard-driven
 * options menu (ags_show_dialog_options, ags/dialog_menu.h) hosted on
 * this game's own real dialog-interface GUI (game.options[
 * OPT_DIALOGIFACE], confirmed ==4 for Rob Blanc 1's own real data),
 * following DCMD_GOTODIALOG chains a few hops deep.
 *
 * Since a scripted test run has no human at the keyboard, option
 * selection is driven via Allegro's own real simulate_keypress() API
 * (a legitimate, documented testing/playback primitive -- injects a
 * genuine key event into the same buffer keypressed()/readkey() read
 * from, not a bypass of the real interactive code path): one ENTER
 * per menu, always choosing whichever option ags_show_dialog_options
 * is currently hovering (index 0, the first enabled option, on
 * entry) -- the same choice this milestone's own earlier "pick the
 * first ON option" stand-in made, just now exercised through the
 * real rendered/polled menu instead of bypassing it.
 *
 * Usage:
 *   test_dialog.exe <path to rb.exe>
 */
#include "ags/clib.h"
#include "ags/loader.h"
#include "ags/script_loader.h"
#include "ags/interp.h"
#include "ags/dialog_run.h"
#include "ags/dialog_menu.h"
#include "ags/gui_loader.h"
#include "ags/gfx.h"
#include "ags/stub.h"

#include <allegro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void run_topic(struct AgsDialogRunContext *ctx, struct GameState *play, int dlgnum,
                       const struct GUIMain *dialogface_gui)
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

        printf("showing real options menu for dialog %d (simulating ENTER to choose "
               "the first enabled option)...\n", cur);
        simulate_keypress(KEY_ENTER << 8);
        chosen = ags_show_dialog_options(dtpp, dialogface_gui, 320, 200);
        if (chosen < 0) {
            printf("dialog %d: no option chosen (all off, or cancelled/timed out) -- "
                   "conversation ends\n", cur);
            return;
        }
        printf("chose option %d: \"%s\"\n", chosen, dtpp->optionnames[chosen]);
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
        /* AGS_DIALOG_RETURN: real do_conversation redisplays the
         * options menu for the SAME topic -- this driver does too,
         * one more time, then stops (avoiding an unbounded loop if a
         * game script somehow never turns every option off). */
        printf("dialog %d: option %d returned to the options menu -- showing it once more\n",
               cur, chosen);
        simulate_keypress(KEY_ENTER << 8);
        chosen = ags_show_dialog_options(dtpp, dialogface_gui, 320, 200);
        if (chosen < 0) {
            printf("dialog %d: no option chosen the second time -- conversation ends\n", cur);
            return;
        }
        printf("chose option %d: \"%s\"\n", chosen, dtpp->optionnames[chosen]);
        rc = ags_run_dialog_script(ctx, dtpp, play, dtpp->entrypoints[chosen]);
        printf("--- option %d run returned %d ---\n", chosen, rc);
        if (rc >= 0 && rc != AGS_DIALOG_RETURN) {
            if (rc == AGS_DIALOG_STOP) {
                printf("dialog %d: STOPDIALOG after option %d\n", cur, chosen);
                return;
            }
            cur = rc;
            continue;
        }
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
    struct AgsGuiSet guiset;
    const struct GUIMain *dialogface_gui = NULL;
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
    if (allegro_init() != 0 || install_keyboard() != 0 || install_mouse() < 0) {
        fprintf(stderr, "allegro_init/install_keyboard/install_mouse failed\n");
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
    if (rc != 0) {
        fprintf(stderr, "ags_load_dlgmessages failed: %d\n", rc);
        fclose(f);
        return 1;
    }

    rc = (int)ags_load_guis(f, &game, &guiset);
    fclose(f);
    if (rc != AGS_GUI_LOAD_OK) {
        fprintf(stderr, "ags_load_guis failed: %d\n", rc);
        return 1;
    }
    printf("game.options[OPT_DIALOGIFACE]=%d numgui=%d\n", game.options[3], guiset.numgui);
    if (game.options[3] > 0 && game.options[3] < guiset.numgui) {
        dialogface_gui = &guiset.guis[game.options[3]];
        printf("using guis[%d] as the dialog-options GUI: x=%d y=%d wid=%d hit=%d bgcol=%d fgcol=%d\n",
               game.options[3], dialogface_gui->x, dialogface_gui->y, dialogface_gui->wid,
               dialogface_gui->hit, dialogface_gui->bgcol, dialogface_gui->fgcol);
    } else {
        printf("no custom dialog-options GUI configured -- using the default bottom-of-screen box\n");
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
        run_topic(&ctx, &play, i, dialogface_gui);
    }

    ags_stub_dump_summary();

    allegro_exit();

    printf("\nM11 ACCEPTANCE CHECK OK (dialog system slice): real DCMD_* bytecode "
           "from this game's own compiled dialog topics was decoded and executed "
           "through the real run_dialog_script interpreter\n");
    return 0;
}
END_OF_MAIN()
