#ifndef ULTIMA6_ULTIMA6_UI_H
#define ULTIMA6_ULTIMA6_UI_H

#include "engine/framebuffer.h"
#include "files/tile_data.h"
#include "ultima6/world.h"

#include <string>

namespace Ultima6 {
namespace Ultima6 {

/** Drawing helpers for the play screen's panels (the driver's draw_tile and draw_char entries). */
namespace Ui {

/** Draws a 16 x 16 tile directly on the screen (driver entry 15). */
void drawTile(Framebuffer &screen, const World &world, int tile, int x, int y);

/** Draws text in the 8 x 8 font at a pixel position; bg < 0 leaves the background alone. */
void drawText(Framebuffer &screen, const Font &font, int x, int y, const std::string &text, int fg, int bg = -1);

/** Copies a rectangle of the paper background over the screen (to wipe a panel). */
void restorePaper(Framebuffer &screen, const Picture &paper, int x, int y, int w, int h);

} // namespace Ui

} // namespace Ultima6
} // namespace Ultima6

#endif
