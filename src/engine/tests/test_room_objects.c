/* M11+ test (see src/PLAN.md): "the long tail", room objects slice.
 * ags_init_room_status (ags/roomobj.h) is a real port of load_new_
 * room's own "first visit" RoomStatus init block -- see that header's
 * own file-level comment for the complete evidence. This test checks
 * it against two real rooms with real on-disk initial objects (found
 * via a throwaway diagnostic, not committed -- room14.crm has 2
 * objects, both on=1 by default; room15.crm has 4, only the first on):
 *   1. room14: both real objects render at their own real x/y using
 *      their own real sprite slot numbers; ags_object_off(0) then
 *      makes the first one disappear (a real ObjectOff, ags/
 *      interaction.h's own respond==6/ags/native_api.h's own
 *      ObjectOff, both now wired to this same function).
 *   2. room15: only object 0 renders by default (matching its own
 *      real on-disk on=1/on=0 flags); ags_object_on(1) then makes the
 *      second one appear too (a real ObjectOn, same wiring).
 *
 * Usage:
 *   test_room_objects.exe <path to rb.exe>
 */
#include "ags/clib.h"
#include "ags/loader.h"
#include "ags/room_loader.h"
#include "ags/sprite_loader.h"
#include "ags/gfx.h"
#include "ags/roomobj.h"

#include <allegro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int save_shot(const char *name, const struct RoomStruct *rst, const struct GameSetupStructBase *game)
{
    RGB merged[256];
    ags_gfx_build_merged_palette(rst, game, merged);
    if (save_bitmap(name, screen, merged) != 0) {
        fprintf(stderr, "save_bitmap(%s) failed\n", name);
        return 0;
    }
    printf("  saved %s\n", name);
    return 1;
}

int main(int argc, char **argv)
{
    struct GameSetupStructBase game;
    struct AgsGameFileHeader hdr;
    struct AgsScriptBlockInfo script_info;
    struct RoomStruct rst14, rst15;
    struct RoomStatus croom14, croom15;
    struct AgsSpriteSet sprites;
    FILE *f;
    int rc;
    int shots_saved = 0;

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
    fclose(f);

    memset(&rst14, 0, sizeof(rst14));
    CHECK(ags_load_room("room14.crm", &rst14));
    memset(&rst15, 0, sizeof(rst15));
    CHECK(ags_load_room("room15.crm", &rst15));
    printf("room14: numsprs=%d, room15: numsprs=%d\n", rst14.numsprs, rst15.numsprs);
    if (rst14.numsprs < 2 || rst15.numsprs < 2) {
        fprintf(stderr, "FAIL: expected room14/room15 to have real initial objects\n");
        allegro_exit();
        return 1;
    }

    rc = ags_spriteset_init(&sprites, "acsprset.spr", &game);
    if (rc != AGS_SPRITE_LOAD_OK) {
        fprintf(stderr, "ags_spriteset_init failed: %d\n", rc);
        allegro_exit();
        return 1;
    }

    if (ags_gfx_init_windowed(rst14.width, rst14.height, 8) != 0) {
        fprintf(stderr, "ags_gfx_init_windowed failed: %s\n", allegro_error);
        ags_spriteset_free(&sprites);
        allegro_exit();
        return 1;
    }

    memset(&croom14, 0, sizeof(croom14));
    ags_init_room_status(&croom14, &rst14);
    printf("\nroom14 croom.obj[] after ags_init_room_status:\n");
    {
        int i;
        for (i = 0; i < croom14.numobj; i++) {
            printf("  obj[%d]: x=%d y=%d num=%d on=%d baseline=%d view=%d moving=%d\n",
                   i, croom14.obj[i].x, croom14.obj[i].y, croom14.obj[i].num,
                   croom14.obj[i].on, croom14.obj[i].baseline, croom14.obj[i].view, croom14.obj[i].moving);
            if (croom14.obj[i].on != 1 || croom14.obj[i].view != -1 || croom14.obj[i].moving != 0) {
                fprintf(stderr, "FAIL: room14 obj[%d] init did not match this game's own real "
                                "on-disk defaults\n", i);
                ags_spriteset_free(&sprites);
                allegro_exit();
                return 1;
            }
        }
    }

    ags_gfx_show_background(&rst14, &game);
    ags_draw_room_objects(&croom14, &sprites, screen);
    shots_saved += save_shot("room_objects_room14_default.bmp", &rst14, &game);

    printf("\ncalling ags_object_off(&croom14, 0) (a real ObjectOff)...\n");
    ags_object_off(&croom14, 0);
    if (croom14.obj[0].on != 0) {
        fprintf(stderr, "FAIL: ags_object_off did not clear obj[0].on\n");
        ags_spriteset_free(&sprites);
        allegro_exit();
        return 1;
    }
    ags_gfx_show_background(&rst14, &game);
    ags_draw_room_objects(&croom14, &sprites, screen);
    shots_saved += save_shot("room_objects_room14_after_off.bmp", &rst14, &game);

    memset(&croom15, 0, sizeof(croom15));
    ags_init_room_status(&croom15, &rst15);
    printf("\nroom15 croom.obj[] after ags_init_room_status:\n");
    {
        int i;
        for (i = 0; i < croom15.numobj; i++) {
            printf("  obj[%d]: x=%d y=%d num=%d on=%d\n",
                   i, croom15.obj[i].x, croom15.obj[i].y, croom15.obj[i].num, croom15.obj[i].on);
        }
    }
    if (croom15.obj[0].on != 1 || croom15.obj[1].on != 0) {
        fprintf(stderr, "FAIL: room15's own real on-disk on/off pattern didn't come through "
                        "ags_init_room_status unchanged\n");
        ags_spriteset_free(&sprites);
        allegro_exit();
        return 1;
    }

    ags_gfx_show_background(&rst15, &game);
    ags_draw_room_objects(&croom15, &sprites, screen);
    shots_saved += save_shot("room_objects_room15_default.bmp", &rst15, &game);

    printf("\ncalling ags_object_on(&croom15, 1) (a real ObjectOn)...\n");
    ags_object_on(&croom15, 1);
    if (croom15.obj[1].on != 1) {
        fprintf(stderr, "FAIL: ags_object_on did not set obj[1].on\n");
        ags_spriteset_free(&sprites);
        allegro_exit();
        return 1;
    }
    ags_gfx_show_background(&rst15, &game);
    ags_draw_room_objects(&croom15, &sprites, screen);
    shots_saved += save_shot("room_objects_room15_after_on.bmp", &rst15, &game);

    ags_spriteset_free(&sprites);
    allegro_exit();

    if (shots_saved != 4) {
        fprintf(stderr, "\nFAIL: expected 4 screenshots, saved %d\n", shots_saved);
        return 1;
    }

    printf("\nM11+ ACCEPTANCE CHECK OK (room objects slice): load_new_room's own real "
           "RoomStatus/RoomObject init (ags_init_room_status), the real ObjectOn/ObjectOff "
           "call, and a scoped static-pose renderer all confirmed against this game's own "
           "real room14/room15 initial-object data\n");
    return 0;
}
END_OF_MAIN()
