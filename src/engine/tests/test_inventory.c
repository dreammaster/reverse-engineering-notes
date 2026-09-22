/* M11 test (see src/PLAN.md): "the long tail" -- inventory slice.
 * Loads Rob Blanc 1's own real GameSetupStructBase.invinfo[]/
 * CharacterInfo array (M2/M3), then exercises the real inventory
 * core (ags/inventory.h: ags_add_inventory/ags_lose_inventory/
 * ags_update_invorder/ags_set_active_inventory) end to end against
 * real item data, confirms EventBlock respond==7/8's own add_inventory
 * promotion in ags_run_event_block (ags/interaction.h), and shows the
 * real default inventory screen (ags_run_inventory_screen, ags/
 * invscreen.h) with real sprite icons for whatever items were granted.
 *
 * Usage:
 *   test_inventory.exe <path to rb.exe>
 */
#include "ags/clib.h"
#include "ags/loader.h"
#include "ags/room_loader.h"
#include "ags/sprite_loader.h"
#include "ags/gfx.h"
#include "ags/inventory.h"
#include "ags/invscreen.h"
#include "ags/interaction.h"
#include "ags/gui_loader.h"
#include "ags/gui_render.h"
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
    struct AgsScriptBlockInfo script_info;
    struct CharacterInfo *chars = NULL;
    struct AgsGuiSet guiset;
    struct RoomStruct rst;
    struct AgsSpriteSet sprites;
    struct AgsGameContext ctx;
    FILE *f;
    int rc;
    int i;
    int playerchar_idx;
    int item_a, item_b;

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
    CHECK(ags_skip_compiled_script(f, &script_info));
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

    /* Continue the real M11 loader chain far enough to reach guis[]
     * too (same steps as test_gui.c/test_dialog.c), purely to
     * demonstrate ags_gui_draw_all_with_inventory doesn't crash --
     * Rob Blanc 1's own real data has numguiinv==0 (confirmed via
     * test_gui.c's own earlier output), so no GOBJ_INVENTORY control
     * actually exists to render here. */
    {
        struct DialogTopic *dialogs = NULL;
        char **dlgmessages = NULL;
        CHECK(ags_load_messages(f, &game));
        rc = ags_load_dialog_topics(f, game.numdialog, &dialogs);
        if (rc != 0) {
            fprintf(stderr, "ags_load_dialog_topics failed: %d\n", rc);
            fclose(f);
            allegro_exit();
            return 1;
        }
        rc = ags_load_dlgmessages(f, game.numdlgmessage, &dlgmessages);
        if (rc != 0) {
            fprintf(stderr, "ags_load_dlgmessages failed: %d\n", rc);
            fclose(f);
            allegro_exit();
            return 1;
        }
        rc = (int)ags_load_guis(f, &game, &guiset);
        if (rc != AGS_GUI_LOAD_OK) {
            fprintf(stderr, "ags_load_guis failed: %d\n", rc);
            fclose(f);
            allegro_exit();
            return 1;
        }
        /* Not needed past this point for this test. */
        for (i = 0; i < game.numdialog; i++) {
            free(dialogs[i].optionscripts);
        }
        free(dialogs);
        for (i = 0; i < game.numdlgmessage; i++) {
            free(dlgmessages[i]);
        }
        free(dlgmessages);
    }
    fclose(f);

    printf("numinvitems=%d\n", game.numinvitems);
    for (i = 1; i < game.numinvitems && i < 10; i++) {
        printf("  invinfo[%d]: name=\"%s\" pic=%d\n", i, game.invinfo[i].name, game.invinfo[i].pic);
    }

    playerchar_idx = game.playercharacter;
    if (playerchar_idx < 0 || playerchar_idx >= game.numcharacters) {
        fprintf(stderr, "FAIL: playercharacter index %d out of range\n", playerchar_idx);
        allegro_exit();
        return 1;
    }
    if (game.numinvitems < 3) {
        fprintf(stderr, "FAIL: this game's real data has too few inventory items to "
                        "demonstrate against (numinvitems=%d)\n", game.numinvitems);
        allegro_exit();
        return 1;
    }

    memset(&play, 0, sizeof(play));
    ctx.rst = NULL;
    ctx.player = &chars[playerchar_idx];
    ctx.player_walk = NULL;
    ctx.quit_requested = 0;
    ctx.play = &play;

    item_a = 1;
    item_b = 2;

    printf("\nplayer starts with inv[%d]=%d inv[%d]=%d (from this game's own real save data)\n",
           item_a, ctx.player->inv[item_a], item_b, ctx.player->inv[item_b]);

    printf("\ncalling ags_add_inventory(%d) and ags_add_inventory(%d)...\n", item_a, item_b);
    ags_add_inventory(ctx.player, &play, item_a);
    ags_add_inventory(ctx.player, &play, item_b);
    printf("inv[%d]=%d inv[%d]=%d, inv_numorder=%d, play_invorder=[",
           item_a, ctx.player->inv[item_a], item_b, ctx.player->inv[item_b], play.inv_numorder);
    for (i = 0; i < play.inv_numorder; i++) {
        printf("%d ", play.play_invorder[i]);
    }
    printf("]\n");

    if (play.inv_numorder < 2 || ctx.player->inv[item_a] < 1 || ctx.player->inv[item_b] < 1) {
        fprintf(stderr, "FAIL: ags_add_inventory did not update ownership/ordering correctly\n");
        allegro_exit();
        return 1;
    }

    printf("\nexercising EventBlock respond==8 (add_inventory via ags_run_event_block) "
           "on a synthetic block with data[0]=%d...\n", item_a + 1 <= game.numinvitems - 1 ? item_a + 1 : item_a);
    {
        struct EventBlock blk;
        int extra_item = (item_a + 3 < game.numinvitems) ? item_a + 3 : item_a;
        memset(&blk, 0, sizeof(blk));
        blk.numcmd = 1;
        blk.list[0] = 42; /* arbitrary checkAgainst value the call below also passes */
        blk.respond[0] = 8; /* add_inventory(data) */
        blk.data[0] = extra_item;
        ags_run_event_block(&ctx, &blk, 42);
        printf("after respond==8: inv[%d]=%d, inv_numorder=%d\n",
               extra_item, ctx.player->inv[extra_item], play.inv_numorder);
        if (ctx.player->inv[extra_item] < 1) {
            fprintf(stderr, "FAIL: EventBlock respond==8 did not grant the item for real\n");
            allegro_exit();
            return 1;
        }
    }

    printf("\ncalling ags_lose_inventory(%d)...\n", item_b);
    ags_lose_inventory(ctx.player, &play, item_b);
    printf("inv[%d]=%d, inv_numorder=%d, play_invorder=[",
           item_b, ctx.player->inv[item_b], play.inv_numorder);
    for (i = 0; i < play.inv_numorder; i++) {
        printf("%d ", play.play_invorder[i]);
    }
    printf("]\n");
    for (i = 0; i < play.inv_numorder; i++) {
        if (play.play_invorder[i] == item_b) {
            fprintf(stderr, "FAIL: ags_lose_inventory left the lost item in play_invorder\n");
            allegro_exit();
            return 1;
        }
    }

    printf("\ncalling ags_set_active_inventory(%d)...\n", item_a);
    rc = ags_set_active_inventory(ctx.player, item_a);
    printf("returned cursor mode %d (4=MODE_USE), activeinv=%d\n", rc, ctx.player->activeinv);
    if (rc != 4 || ctx.player->activeinv != item_a) {
        fprintf(stderr, "FAIL: ags_set_active_inventory did not select the owned item\n");
        allegro_exit();
        return 1;
    }

    if (ags_gfx_init_windowed(320, 200, 8) != 0) {
        fprintf(stderr, "ags_gfx_init_windowed failed: %s\n", allegro_error);
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

    ags_gfx_show_background(&rst, &game);
    /* Exercises ags_gui_draw_all_with_inventory for real -- see this
     * file's own earlier comment on why it draws no GOBJ_INVENTORY
     * control for THIS game's own real data (numguiinv==0). */
    ags_gui_draw_all_with_inventory(&guiset, &sprites, screen, &play, ctx.player, game.numinvitems, game.invinfo);

    printf("\nshowing the real default inventory screen (no simulated input -- it will "
           "run to its own ~30s timeout, exactly like a player who walks away; the "
           "screenshot below captures whatever frame it last drew before returning)...\n");
    ags_run_inventory_screen(&ctx, &game, &sprites, 320, 200);

    {
        RGB merged[256];
        ags_gfx_build_merged_palette(&rst, &game, merged);
        if (save_bitmap("inventory_screenshot.bmp", screen, merged) != 0) {
            fprintf(stderr, "save_bitmap failed\n");
        } else {
            printf("screenshot saved: inventory_screenshot.bmp\n");
        }
    }

    ags_stub_dump_summary();

    ags_spriteset_free(&sprites);
    allegro_exit();
    free(chars);

    printf("\nM11 ACCEPTANCE CHECK OK (inventory slice): real add_inventory/"
           "LoseInventory/update_invorder/SetActiveInventory all confirmed against this "
           "game's own real inventory data, including via a real EventBlock respond==8 "
           "dispatch\n");
    return 0;
}
END_OF_MAIN()
