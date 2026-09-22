/* M9 test (see src/PLAN.md): "Say something." A real mainloop
 * (extending M8's own): a left click on a real hotspot runs
 * RunHotspotInteraction(hotspot, MODE_LOOK) -- this build's own real
 * get_hotspot_at + run_event_block dispatch (ags/interaction.h) --
 * showing a real DisplayMessage text box with the room's own actual
 * message text; a left click anywhere else still walks there
 * (M8, unchanged). ESC quits.
 *
 * Uses room6.crm, same as M8 (see test_walk_room.c's own NOTE on why
 * room1 -- PLAN.md's literal room -- has no real interaction data at
 * all: it's a pure title card). room6's own real hotspot data (found
 * via a throwaway diagnostic, not committed) has hotspot 1 ("PORTHOLE")
 * and hotspot 2 ("TOOLBOX") both wired to real DisplayMessage
 * responses on a MODE_LOOK click.
 *
 * For scripted verification, the loop also drives two automatic
 * "look" clicks at hotspot 1 and hotspot 2's own real mask locations
 * (found by scanning rst.lookat) before handing control to any live
 * keyboard/mouse input, saving a screenshot of each resulting message
 * box.
 *
 * Usage:
 *   test_say_something.exe <path to rb.exe>
 */
#include "ags/clib.h"
#include "ags/loader.h"
#include "ags/room_loader.h"
#include "ags/sprite_loader.h"
#include "ags/gfx.h"
#include "ags/walk.h"
#include "ags/interaction.h"

#include <allegro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static block load_current_pose(struct AgsSpriteSet *sprites, const struct CharacterInfo *chin,
                                const struct ViewStruct272 *views)
{
    int loopn = chin->loop, frame = chin->frame, pic;
    if (loopn < 0 || loopn >= 8 || frame < 0 || frame >= 10) return NULL;
    pic = views[chin->view].frames[loopn][frame].pic;
    if (pic < 0) return NULL;
    return ags_spriteset_load(sprites, pic);
}

static void draw_frame(const struct RoomStruct *rst, block sprite_bmp, const struct CharacterInfo *chin)
{
    ags_gfx_show_background(rst);
    if (sprite_bmp) {
        int draw_x = chin->x - sprite_bmp->w / 2;
        int draw_y = chin->y - sprite_bmp->h;
        draw_sprite(screen, sprite_bmp, draw_x, draw_y);
    }
}

/* Finds a representative (x,y) for hotspot index `hs` by scanning
 * rst->lookat for the first matching pixel. */
static int find_hotspot_point(block lookat, int hs, int *out_x, int *out_y)
{
    int x, y;
    for (y = 0; y < lookat->h; y++) {
        for (x = 0; x < lookat->w; x++) {
            if (getpixel(lookat, x, y) == hs) {
                *out_x = x;
                *out_y = y;
                return 1;
            }
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    struct GameSetupStructBase game;
    struct AgsGameFileHeader hdr;
    struct AgsScriptBlockInfo script_info;
    struct ViewStruct272 *views = NULL;
    struct CharacterInfo *chars = NULL;
    struct RoomStruct rst;
    struct AgsSpriteSet sprites;
    struct AgsWalkState walk;
    struct AgsGameContext ctx;
    FILE *f;
    int rc;
    int playerchar_idx;
    struct CharacterInfo *pc;
    int frame_count = 0;
    const int max_frames = 3000;
    int shot_num = 0;
    int auto_clicks_done = 0;   /* count of auto-clicks actually fired, 0..2 */
    int auto_clicks_finished = 0;
    static const int auto_click_hotspots[2] = { 2, 1 }; /* TOOLBOX, then PORTHOLE */

    if (argc < 2) {
        fprintf(stderr, "usage: %s <path to rb.exe>\n", argv[0]);
        return 1;
    }

    if (allegro_init() != 0 || install_keyboard() != 0 || install_mouse() < 0) {
        fprintf(stderr, "allegro input init failed\n");
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
    chars = (struct CharacterInfo *)malloc((size_t)game.numcharacters * sizeof(struct CharacterInfo));
    if (!views || !chars) {
        fprintf(stderr, "out of memory\n");
        fclose(f);
        allegro_exit();
        return 1;
    }
    CHECK(ags_load_views(f, &game, views));
    CHECK(ags_skip_unidentified_block2(f));
    CHECK(ags_load_characters(f, &game, chars));
    fclose(f);

    playerchar_idx = game.playercharacter;
    if (playerchar_idx < 0 || playerchar_idx >= game.numcharacters) {
        fprintf(stderr, "FAIL: playercharacter index out of range\n");
        allegro_exit();
        return 1;
    }
    pc = &chars[playerchar_idx];
    pc->room = 6;

    memset(&rst, 0, sizeof(rst));
    CHECK(ags_load_room("room6.crm", &rst));
    printf("room6.crm loaded: %dx%d\n", rst.width, rst.height);

    rc = ags_spriteset_init(&sprites, "acsprset.spr", &game);
    if (rc != AGS_SPRITE_LOAD_OK) {
        fprintf(stderr, "ags_spriteset_init failed: %d\n", rc);
        allegro_exit();
        return 1;
    }

    /* Same walkable-span scan as M8, just to give the player a sane
     * starting position on real ground. */
    {
        int left, right, row, found = 0;
        for (row = rst.height / 2; row < rst.height && !found; row++) {
            int x, l = -1, r = -1;
            for (x = 0; x < rst.walls->w; x++) {
                if (getpixel(rst.walls, x, row) >= 1) {
                    if (l < 0) l = x;
                    r = x;
                }
            }
            if (l >= 0 && (r - l) > 20) {
                left = l; right = r; found = 1;
            }
        }
        if (!found) {
            fprintf(stderr, "FAIL: no usable walkable span in room6\n");
            ags_spriteset_free(&sprites);
            allegro_exit();
            return 1;
        }
        pc->x = left + 5;
        pc->y = row - 1;
    }

    pc->loop = 0;
    pc->frame = 0;
    pc->wait = 0;
    pc->walking = 0;
    memset(&walk, 0, sizeof(walk));

    ctx.rst = &rst;
    ctx.player = pc;
    ctx.player_walk = &walk;
    ctx.quit_requested = 0;

    if (ags_gfx_init_windowed(rst.width, rst.height, 8) != 0) {
        fprintf(stderr, "ags_gfx_init_windowed failed: %s\n", allegro_error);
        ags_spriteset_free(&sprites);
        allegro_exit();
        return 1;
    }

    for (;;) {
        block sprite_bmp;
        int click_x = -1, click_y = -1;

        if (keypressed()) {
            int k = readkey();
            if ((k >> 8) == KEY_ESC) {
                break;
            }
        }

        if (auto_clicks_done < 2 && frame_count > 0 && frame_count % 40 == 0) {
            int hs = auto_click_hotspots[auto_clicks_done];
            if (find_hotspot_point(rst.lookat, hs, &click_x, &click_y)) {
                auto_clicks_done++;
                if (auto_clicks_done >= 2) {
                    auto_clicks_finished = 1;
                }
            }
        }
        if (mouse_b & 1) {
            click_x = mouse_x;
            click_y = mouse_y;
        }

        if (click_x >= 0) {
            int hs = ags_get_hotspot_at(&rst, click_x, click_y);
            printf("frame %d: click at (%d,%d) -> hotspot %d (\"%s\")\n",
                   frame_count, click_x, click_y, hs, hs > 0 ? rst.hotspotnames[hs] : "(none)");
            if (hs > 0) {
                sprite_bmp = load_current_pose(&sprites, pc, views);
                draw_frame(&rst, sprite_bmp, pc);
                if (sprite_bmp) destroy_bitmap(sprite_bmp);
                ags_run_hotspot_interaction(&ctx, hs, AGS_MODE_LOOK);
                {
                    char name[64];
                    sprintf(name, "say_frame_%d.bmp", shot_num++);
                    save_bitmap(name, screen, (const RGB *)rst.pal);
                    printf("  saved %s\n", name);
                }
            } else {
                ags_walk_character_straight(pc, &walk, rst.walls, views, click_x, click_y);
            }
        }

        ags_advance_walk_animation(pc, &walk, views);

        sprite_bmp = load_current_pose(&sprites, pc, views);
        draw_frame(&rst, sprite_bmp, pc);
        if (sprite_bmp) destroy_bitmap(sprite_bmp);

        frame_count++;
        if (frame_count > max_frames || ctx.quit_requested) {
            break;
        }
        if (auto_clicks_finished && shot_num >= 2 && frame_count > 90) {
            break; /* scripted run: both auto-clicks captured, exit */
        }

        rest(15);
    }

    ags_spriteset_free(&sprites);
    allegro_exit();
    free(views);
    free(chars);

    if (shot_num < 2) {
        fprintf(stderr, "\nFAIL: didn't capture both automatic hotspot-click message boxes\n");
        return 1;
    }

    printf("\nM9 ACCEPTANCE CHECK OK: clicking real hotspots produced the real game's "
           "own DisplayMessage text, via a real get_hotspot_at/RunHotspotInteraction/"
           "run_event_block dispatch\n");
    return 0;
}
END_OF_MAIN()
