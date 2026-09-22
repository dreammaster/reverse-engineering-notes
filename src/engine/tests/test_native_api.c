/* M11 test (see src/PLAN.md): "the long tail" -- the remaining
 * script-API entries slice. Loads Rob Blanc 1's own real compiled
 * global script, room, characters, and sprites, wires up a real
 * ags_native_api_set_context (ags/native_api.h), and then actually
 * RUNS the real script's game_start()/repeatedly_execute()/
 * on_mouse_click() exports through the real interpreter (M4) --
 * confirming that SCMD_CALLEXT's own newly-real dispatch table
 * genuinely mutates THIS engine's own real GameState/CharacterInfo
 * data when the compiled script calls a native function, not just
 * this project's own test harnesses calling the same C function
 * directly.
 *
 * Usage:
 *   test_native_api.exe <path to rb.exe>
 */
#include "ags/clib.h"
#include "ags/loader.h"
#include "ags/script_loader.h"
#include "ags/interp.h"
#include "ags/room_loader.h"
#include "ags/sprite_loader.h"
#include "ags/gfx.h"
#include "ags/native_api.h"
#include "ags/stub.h"

#include <allegro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct GameSetupStructBase game;
    struct GameState play;
    struct AgsGameFileHeader hdr;
    struct CharacterInfo *chars = NULL;
    struct RoomStruct rst;
    struct AgsSpriteSet sprites;
    struct AgsGameContext game_ctx;
    struct AgsNativeApiContext native_ctx;
    struct ccScript *scri = NULL;
    struct ccInstance *inst = NULL;
    enum AgsScriptLoadError script_err;
    FILE *f;
    int rc;
    int i;
    int playerchar_idx;
    long retval;
    int run_rc;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <path to rb.exe>\n", argv[0]);
        return 1;
    }

    if (allegro_init() != 0 || install_keyboard() != 0 || install_mouse() < 0) {
        fprintf(stderr, "allegro_init/install_keyboard/install_mouse failed\n");
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
        allegro_exit();
        return 1;
    }

#define CHECK(call) \
    do { \
        rc = (call); \
        if (rc != 0) { \
            fprintf(stderr, #call " failed: %d\n", rc); \
            fclose(f); \
            allegro_exit(); \
            return 1; \
        } \
    } while (0)

    CHECK(ags_load_game_file_header(f, &hdr));
    CHECK(ags_load_gamesetup(f, &game));
    CHECK(ags_skip_words_dictionary(f, &game));
    CHECK(ags_skip_unidentified_block(f));

    scri = ags_cc_read_script(f, &script_err);
    if (!scri) {
        fprintf(stderr, "ags_cc_read_script failed: %d\n", (int)script_err);
        fclose(f);
        allegro_exit();
        return 1;
    }
    CHECK(ags_skip_views(f, &game));
    CHECK(ags_skip_unidentified_block2(f));

    chars = (struct CharacterInfo *)malloc((size_t)game.numcharacters * sizeof(struct CharacterInfo));
    if (!chars) {
        fprintf(stderr, "out of memory (chars)\n");
        fclose(f);
        allegro_exit();
        return 1;
    }
    CHECK(ags_load_characters(f, &game, chars));
    fclose(f);

    playerchar_idx = game.playercharacter;
    if (playerchar_idx < 0 || playerchar_idx >= game.numcharacters) {
        fprintf(stderr, "FAIL: playercharacter index %d out of range\n", playerchar_idx);
        allegro_exit();
        return 1;
    }

    memset(&rst, 0, sizeof(rst));
    CHECK(ags_load_room("room6.crm", &rst));

    rc = ags_spriteset_init(&sprites, "acsprset.spr", &game);
    if (rc != AGS_SPRITE_LOAD_OK) {
        fprintf(stderr, "ags_spriteset_init failed: %d\n", rc);
        allegro_exit();
        return 1;
    }
    if (ags_gfx_init_windowed(320, 200, 8) != 0) {
        fprintf(stderr, "ags_gfx_init_windowed failed: %s\n", allegro_error);
        allegro_exit();
        return 1;
    }
    ags_gfx_show_background(&rst, &game);

    memset(&play, 0, sizeof(play));

    game_ctx.rst = &rst;
    game_ctx.player = &chars[playerchar_idx];
    game_ctx.player_walk = NULL;
    game_ctx.quit_requested = 0;
    game_ctx.play = &play;

    native_ctx.game_ctx = &game_ctx;
    native_ctx.game = &game;
    native_ctx.sprites = &sprites;
    native_ctx.dialog_ctx = NULL;
    native_ctx.chars = chars;
    native_ctx.numcharacters = game.numcharacters;
    native_ctx.screen_w = 320;
    native_ctx.screen_h = 200;
    native_ctx.game_paused = 0;
    native_ctx.cur_cursor = 0;
    ags_native_api_set_context(&native_ctx);

    inst = ags_cc_create_instance(scri);
    if (!inst) {
        fprintf(stderr, "FAIL: ags_cc_create_instance returned NULL\n");
        allegro_exit();
        return 1;
    }

    printf("before game_start(): disabled_user_interface=%d\n", play.disabled_user_interface);
    printf("\nrunning game_start() through the real interpreter, with a real native "
           "dispatch context wired up...\n");
    retval = -12345;
    run_rc = ags_cc_call_instance(inst, "game_start", 0, NULL, &retval);
    printf("game_start() returned %d, AX=%ld\n", run_rc, retval);
    if (run_rc != AGS_CC_RUN_OK) {
        fprintf(stderr, "FAIL: game_start() did not run to completion (%d)\n", run_rc);
        allegro_exit();
        return 1;
    }
    printf("after game_start(): disabled_user_interface=%d (expected 2, this game's own "
           "real InterfaceOff() call count -- an EARLIER apparent count of 4, from before "
           "this same round's own ags_stub_hit dedup-key fix, turned out to be wrong: "
           "every distinct native call name sharing CALLEXT's own constant pseudo-location "
           "was silently being merged into whichever name was logged first)\n",
           play.disabled_user_interface);
    if (play.disabled_user_interface != 2) {
        fprintf(stderr, "FAIL: InterfaceOff's real dispatch did not update the real "
                        "GameState field as expected\n");
        allegro_exit();
        return 1;
    }

    printf("\ncalling repeatedly_execute() a few times (simulating game-loop ticks)...\n");
    for (i = 0; i < 3; i++) {
        run_rc = ags_cc_call_instance(inst, "repeatedly_execute", 0, NULL, &retval);
        printf("  tick %d: returned %d, AX=%ld\n", i, run_rc, retval);
        if (run_rc != AGS_CC_RUN_OK) {
            fprintf(stderr, "FAIL: repeatedly_execute() did not run to completion (%d)\n", run_rc);
            allegro_exit();
            return 1;
        }
    }

    printf("\ncalling on_mouse_click(1) (a real script export this game defines)...\n");
    {
        long arg = 1;
        run_rc = ags_cc_call_instance(inst, "on_mouse_click", 1, &arg, &retval);
        printf("  returned %d, AX=%ld\n", run_rc, retval);
        if (run_rc != AGS_CC_RUN_OK) {
            fprintf(stderr, "FAIL: on_mouse_click() did not run to completion (%d)\n", run_rc);
            allegro_exit();
            return 1;
        }
    }

    printf("\ncalling on_key_press(13) (Enter)...\n");
    {
        long arg = 13;
        run_rc = ags_cc_call_instance(inst, "on_key_press", 1, &arg, &retval);
        printf("  returned %d, AX=%ld\n", run_rc, retval);
        if (run_rc != AGS_CC_RUN_OK) {
            fprintf(stderr, "FAIL: on_key_press() did not run to completion (%d)\n", run_rc);
            allegro_exit();
            return 1;
        }
    }

    ags_stub_dump_summary();

    ags_spriteset_free(&sprites);
    allegro_exit();
    free(chars);

    printf("\nM11 ACCEPTANCE CHECK OK (remaining script-API entries slice): the real "
           "compiled script drove real engine state through SCMD_CALLEXT's own new "
           "dispatch table, confirmed via disabled_user_interface's own real value after "
           "game_start() actually called InterfaceOff() for real, not just logged it\n");
    return 0;
}
END_OF_MAIN()
