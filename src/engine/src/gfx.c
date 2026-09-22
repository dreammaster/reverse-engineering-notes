/* ags/gfx.h's own implementation. See that header's file-level
 * comment for the palette-merge correction.
 */
#include "ags/gfx.h"

/* Common/acroom.h's own confirmed constants. */
#define PAL_BACKGROUND 2

int ags_gfx_init_windowed(int width, int height, int color_depth)
{
    set_color_depth(color_depth);
    return set_gfx_mode(GFX_AUTODETECT_WINDOWED, width, height, 0, 0);
}

void ags_gfx_build_merged_palette(const struct RoomStruct *rst, const struct GameSetupStructBase *game,
                                   RGB out[256])
{
    int i;

    for (i = 0; i < 256; i++) {
        if (game->paluses[i] == PAL_BACKGROUND) {
            out[i].r = rst->pal[i][0];
            out[i].g = rst->pal[i][1];
            out[i].b = rst->pal[i][2];
            out[i].filler = rst->pal[i][3];
        } else {
            const unsigned char *dp = (const unsigned char *)&game->defpal[i];
            out[i].r = dp[0];
            out[i].g = dp[1];
            out[i].b = dp[2];
            out[i].filler = dp[3];
        }
    }
}

void ags_gfx_show_background(const struct RoomStruct *rst, const struct GameSetupStructBase *game)
{
    RGB merged[256];

    ags_gfx_build_merged_palette(rst, game, merged);
    set_palette(merged);

    blit(rst->ebscene[0], screen, 0, 0, 0, 0,
         rst->ebscene[0]->w, rst->ebscene[0]->h);
}
