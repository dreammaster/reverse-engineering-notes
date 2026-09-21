/* M6 test (see src/PLAN.md): "See the room." Loads Rob Blanc 1's real
 * room1.crm (M5), opens a real windowed Allegro graphics mode, and
 * blits the room's decompressed background (with its own real
 * palette) onto the visible screen -- the first milestone that
 * exercises the graphics pipeline (linked since M0, unused until
 * now) end to end. Since this runs as an automated/scripted test
 * rather than someone watching the window live, it also saves a
 * screenshot of the actual `screen` bitmap to disk (room1_screenshot.bmp)
 * so the result can be inspected afterward without needing to catch
 * the window while it's open.
 *
 * Usage:
 *   test_see_room.exe <path to rb.exe>
 */
#include "ags/clib.h"
#include "ags/room_loader.h"
#include "ags/gfx.h"

#include <allegro.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct RoomStruct rst;
    enum AgsRoomLoadError rc;

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

    memset(&rst, 0, sizeof(rst));
    rc = ags_load_room("room1.crm", &rst);
    if (rc != AGS_ROOM_LOAD_OK) {
        fprintf(stderr, "ags_load_room(\"room1.crm\") failed: %d\n", (int)rc);
        allegro_exit();
        return 1;
    }
    printf("room1.crm loaded: %dx%d, wasversion=%d\n", rst.width, rst.height, rst.wasversion);

    /* Rob Blanc 1 is an 8-bit paletted game at its own native 320x200
     * -- see GameSetupStructBase.color_depth (already confirmed
     * elsewhere in this project). No scaling/letterboxing yet, that's
     * later milestone work. */
    if (ags_gfx_init_windowed(rst.width, rst.height, 8) != 0) {
        fprintf(stderr, "ags_gfx_init_windowed failed: %s\n", allegro_error);
        allegro_exit();
        return 1;
    }
    printf("graphics mode opened: %dx%d, color depth %d\n", SCREEN_W, SCREEN_H, bitmap_color_depth(screen));

    ags_gfx_show_background(&rst);

    /* Give the window a moment to actually paint before we grab a
     * screenshot and tear it down -- this is a scripted/automated
     * test, not an interactive session, so we don't wait for a
     * keypress. */
    rest(500);

    if (save_bitmap("room1_screenshot.bmp", screen, (const RGB *)rst.pal) != 0) {
        fprintf(stderr, "save_bitmap failed\n");
        allegro_exit();
        return 1;
    }
    printf("screenshot saved: room1_screenshot.bmp\n");

    allegro_exit();

    printf("\nM6 ACCEPTANCE CHECK OK: room1.crm's real background was blitted onto "
           "a real Allegro window and saved to room1_screenshot.bmp for visual "
           "inspection\n");
    return 0;
}
END_OF_MAIN()
