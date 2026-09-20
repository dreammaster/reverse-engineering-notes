/* M5 test (see src/PLAN.md): "Open a room." Loads Rob Blanc 1's real
 * room1.crm (via ags_load_room, a real port of load_room/
 * load_main_block's block-type dispatch, including real LZW
 * background decompression and real RLE mask decompression -- see
 * ags/room_loader.h's own file-level comment for full scope) and
 * prints room 1's width/height/hotspot names/object count, per
 * PLAN.md's own stated acceptance check for this milestone.
 *
 * Usage:
 *   test_room.exe <path to rb.exe>
 */
#include "ags/clib.h"
#include "ags/room_loader.h"

#include <allegro.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct RoomStruct rst;
    enum AgsRoomLoadError rc;
    int i;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <path to rb.exe>\n", argv[0]);
        return 1;
    }

    /* allegro_init() sets up the internal state create_bitmap_ex/
     * destroy_bitmap need (color-depth tables etc.) -- no graphics
     * MODE is set (no set_gfx_mode call), matching the fact that M5
     * only needs real, off-screen memory bitmaps; wiring one onto an
     * actual visible window is M6's own job. */
    if (allegro_init() != 0) {
        fprintf(stderr, "allegro_init failed\n");
        return 1;
    }
    set_color_depth(8); /* Rob Blanc 1 is an 8-bit paletted game -- GameSetupStructBase.color_depth==1, already confirmed elsewhere in this project */

    if (ags_csetlib(argv[1]) != 0) {
        fprintf(stderr, "ags_csetlib failed\n");
        allegro_exit();
        return 1;
    }

    memset(&rst, 0, sizeof(rst));
    rc = ags_load_room("room1.crm", &rst);
    if (rc != AGS_ROOM_LOAD_OK) {
        fprintf(stderr, "ags_load_room(\"room1.crm\") failed: %d\n", (int)rc);
        allegro_exit();
        return 1;
    }

    printf("room1.crm loaded OK (wasversion=%d, bytes_per_pixel=%d, resolution=%d)\n",
           rst.wasversion, rst.bytes_per_pixel, rst.resolution);
    printf("width=%d height=%d\n", rst.width, rst.height);
    printf("numhotspots=%d\n", rst.numhotspots);
    for (i = 0; i < rst.numhotspots; i++) {
        printf("  hotspot[%d] = \"%s\"\n", i, rst.hotspotnames[i]);
    }
    /* NOTE: RoomStruct.numobj is NOT a room-object count despite the
     * name -- Common/acroom.h's own comment calls it out directly:
     * "numobj; // num hotspots, not sprites" (it's really the walk-
     * behind-area baseline count, objyval[]'s own size). The real
     * "how many objects does this room start with" count is
     * `numsprs` (sprs[], "number of initial sprites"). */
    printf("numsprs (initial object count) = %d\n", rst.numsprs);
    for (i = 0; i < rst.numsprs; i++) {
        printf("  sprs[%d] = {sprnum=%d x=%d y=%d room=%d on=%d}\n", i,
               rst.sprs[i].sprnum, rst.sprs[i].x, rst.sprs[i].y,
               rst.sprs[i].room, rst.sprs[i].on);
    }
    printf("numwalkareas=%d\n", rst.numwalkareas);
    printf("numobj (walk-behind baseline count, NOT object count) = %d\n", rst.numobj);

    printf("\ndecompressed bitmaps: background=%dx%d walls=%dx%d object=%dx%d "
           "lookat=%dx%d regions=%dx%d\n",
           rst.ebscene[0]->w, rst.ebscene[0]->h,
           rst.walls->w, rst.walls->h,
           rst.object->w, rst.object->h,
           rst.lookat->w, rst.lookat->h,
           rst.regions->w, rst.regions->h);

    /* M5's own acceptance check (src/PLAN.md): width/height/hotspot
     * names/object count must come back sane and match the room's
     * real, decompressed data -- not placeholder/stub values. */
    if (rst.width <= 0 || rst.height <= 0 || rst.numhotspots <= 0 ||
        rst.ebscene[0]->w != rst.width || rst.ebscene[0]->h != rst.height) {
        fprintf(stderr, "\nFAIL: room1.crm's decoded fields don't look sane\n");
        allegro_exit();
        return 1;
    }

    printf("\nM5 ACCEPTANCE CHECK OK: room1.crm's width/height/hotspot names/"
           "object count decoded via the real block-dispatch + LZW/RLE "
           "decompression path\n");

    allegro_exit();
    return 0;
}
END_OF_MAIN()
