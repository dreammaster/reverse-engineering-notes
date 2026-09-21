/* ags/gfx.h's own implementation. */
#include "ags/gfx.h"

int ags_gfx_init_windowed(int width, int height, int color_depth)
{
    set_color_depth(color_depth);
    return set_gfx_mode(GFX_AUTODETECT_WINDOWED, width, height, 0, 0);
}

void ags_gfx_show_background(const struct RoomStruct *rst)
{
    set_palette((const RGB *)rst->pal);
    blit(rst->ebscene[0], screen, 0, 0, 0, 0,
         rst->ebscene[0]->w, rst->ebscene[0]->h);
}
