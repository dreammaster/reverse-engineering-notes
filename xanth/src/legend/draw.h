/* Drawing primitives of the original's graphics layer, on a Surface (8-bit palette indices). Rectangles here are inclusive on all edges, like the original's
 * (x1, y1, x2, y2) arguments.
 *
 * Gfx_FrameRect with flag 0x10 shades the pixels already on the screen along the edges (an inset/outset bevel over any background that uses the grey ramp
 * 0x11..0x1F of the interface palette); with flag 4 it fills the rectangle with colour 10 and draws a two-colour raised edge (15 above/left, 6 below/right).
 */
#ifndef LEGEND_DRAW_H
#define LEGEND_DRAW_H

#include "common/types.h"
#include "legend/surface.h"

namespace Legend {

/** Gfx_FillArea with a solid colour. */
void fillArea(Surface &s, int x1, int y1, int x2, int y2, byte color);

/** One pixel of the grey ramp darkened (floor 0x11), as the left and top edges of a bevel do. */
void darkenPixel(Surface &s, int x, int y);
/** One pixel of the grey ramp lightened as the bottom and right edges of a bevel do. */
void lightenPixel(Surface &s, int x, int y);

/** Gfx_FrameRect flag 0x10: an inset bevel made by darkening the pixels on the left and top edge and lightening those on the right and bottom edge along the grey ramp 0x11..0x1F. (With flags 0x14 the original does the opposite with slightly different edge ranges; not needed yet.) */
void bevelFrame(Surface &s, int x1, int y1, int x2, int y2);

/** Gfx_FrameRect flag 4 (raised) or 8 (sunken) without 0x10: colour 10 fill and a line edge (draw_button). */
void buttonFrame(Surface &s, int x1, int y1, int x2, int y2, bool raised);

/** Draws a picture's pixels with its top-left corner at (x, y): a plain copy, or (xorMode, picture flag 0x40) exclusive-or onto the screen. */
void drawPicture(Surface &s, int x, int y, const byte *pixels, int w, int h, bool xorMode);

} // namespace Legend

#endif
