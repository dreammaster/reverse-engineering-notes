/* M7 test (see src/PLAN.md): "Meet the room's people." Walks the full
 * M2/M3 loader chain but with ags_skip_views swapped for the new
 * ags_load_views (a real decode -- M7's own extension to loader.c),
 * finds the player character's own current view/loop/frame, looks up
 * the resulting sprite number in the decoded ViewStruct272[] array,
 * loads that room (M5) and that sprite (M7's new sprite_loader.c),
 * and draws the player's static pose at their own room position on
 * top of the room's background (M6) -- no animation or movement
 * logic, per PLAN.md's own explicit scope for this milestone.
 *
 * Usage:
 *   test_meet_people.exe <path to rb.exe>
 */
#include "ags/clib.h"
#include "ags/loader.h"
#include "ags/room_loader.h"
#include "ags/sprite_loader.h"
#include "ags/gfx.h"

#include <allegro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct GameSetupStructBase game;
    struct AgsGameFileHeader hdr;
    struct AgsScriptBlockInfo script_info;
    struct ViewStruct272 *views = NULL;
    struct CharacterInfo *chars = NULL;
    struct RoomStruct rst;
    struct AgsSpriteSet sprites;
    FILE *f;
    int rc;
    int playerchar_idx;
    int view, loopn, frame, pic;
    block sprite_bmp;
    char roomfile[32];

    if (argc < 2) {
        fprintf(stderr, "usage: %s <path to rb.exe>\n", argv[0]);
        return 1;
    }

    if (allegro_init() != 0) {
        fprintf(stderr, "allegro_init failed\n");
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

    views = (struct ViewStruct272 *)malloc((size_t)game.numviews * sizeof(struct ViewStruct272));
    if (!views) {
        fprintf(stderr, "out of memory (views)\n");
        fclose(f);
        allegro_exit();
        return 1;
    }
    CHECK(ags_load_views(f, &game, views));

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

    {
        struct CharacterInfo *pc = &chars[playerchar_idx];
        view = pc->view;
        loopn = pc->loop;
        frame = pc->frame;
        printf("player character: \"%s\" (index %d), room=%d x=%d y=%d view=%d loop=%d frame=%d\n",
               pc->name, playerchar_idx, pc->room, pc->x, pc->y, view, loopn, frame);

        if (view < 0 || view >= game.numviews) {
            fprintf(stderr, "FAIL: player has no valid current view (%d)\n", view);
            allegro_exit();
            return 1;
        }
        if (loopn < 0 || loopn >= 8 || frame < 0 || frame >= 10) {
            fprintf(stderr, "FAIL: loop/frame out of this build's own 8x10 ViewStruct272 bounds\n");
            allegro_exit();
            return 1;
        }
        printf("view[%d]: numloops=%d numframes[%d]=%d\n", view,
               views[view].numloops, loopn, views[view].numframes[loopn]);

        pic = views[view].frames[loopn][frame].pic;
        printf("sprite number for this pose: %d\n", pic);
        if (pic < 0) {
            fprintf(stderr, "FAIL: that loop/frame slot is unused (pic sentinel -1)\n");
            allegro_exit();
            return 1;
        }

        sprintf(roomfile, "room%d.crm", pc->room);
    }

    memset(&rst, 0, sizeof(rst));
    CHECK(ags_load_room(roomfile, &rst));
    printf("%s loaded: %dx%d\n", roomfile, rst.width, rst.height);

    rc = ags_spriteset_init(&sprites, "acsprset.spr", &game);
    if (rc != AGS_SPRITE_LOAD_OK) {
        fprintf(stderr, "ags_spriteset_init failed: %d\n", rc);
        allegro_exit();
        return 1;
    }

    sprite_bmp = ags_spriteset_load(&sprites, pic);
    if (!sprite_bmp) {
        fprintf(stderr, "FAIL: ags_spriteset_load(%d) returned NULL\n", pic);
        ags_spriteset_free(&sprites);
        allegro_exit();
        return 1;
    }
    printf("sprite %d loaded: %dx%d, color depth %d\n", pic, sprite_bmp->w, sprite_bmp->h,
           bitmap_color_depth(sprite_bmp));

    if (ags_gfx_init_windowed(rst.width, rst.height, 8) != 0) {
        fprintf(stderr, "ags_gfx_init_windowed failed: %s\n", allegro_error);
        destroy_bitmap(sprite_bmp);
        ags_spriteset_free(&sprites);
        allegro_exit();
        return 1;
    }

    ags_gfx_show_background(&rst);

    /* AGS's own character-drawing convention: (x,y) is where the
     * character's feet stand, horizontally centered on the sprite --
     * pure drawing-position math, not movement/animation logic. */
    {
        struct CharacterInfo *pc = &chars[playerchar_idx];
        int draw_x = pc->x - sprite_bmp->w / 2;
        int draw_y = pc->y - sprite_bmp->h;
        draw_sprite(screen, sprite_bmp, draw_x, draw_y);
        printf("drew sprite %d at (%d,%d)\n", pic, draw_x, draw_y);
    }

    rest(500);

    if (save_bitmap("room_with_player.bmp", screen, (const RGB *)rst.pal) != 0) {
        fprintf(stderr, "save_bitmap failed\n");
    } else {
        printf("screenshot saved: room_with_player.bmp\n");
    }

    destroy_bitmap(sprite_bmp);
    ags_spriteset_free(&sprites);
    allegro_exit();

    free(views);
    free(chars);

    printf("\nM7 ACCEPTANCE CHECK OK: the player character's real static view/loop/"
           "frame sprite was drawn at their own room position, on top of their "
           "own real starting room's background\n");
    return 0;
}
END_OF_MAIN()
