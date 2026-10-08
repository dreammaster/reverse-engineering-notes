/* Glue between the game state and the translated 3D view pipeline (view_host.c, gen/view_gen.c):
 * fills the data segment DG[] (party position, maze page, graphics set handles), runs the pipeline and composites the
 * draw lists it produces with the real sprite sheets. */
#ifndef MM3_VIEW_GLUE_H
#define MM3_VIEW_GLUE_H

#include "cc.h"
#include "dgroup.h"
#include "gfx.h"
#include "maze.h"

typedef struct Mm3View Mm3View;

Mm3View *mm3_view_create(const Mm3Cc *mm3cc, const Mm3Cc *cur, const Mm3Dgroup *dg);
void mm3_view_destroy(Mm3View *v);

/* Load a map (1-40: the indoor maps) and put the party on it.  Returns 0 on success. */
int mm3_view_set_map(Mm3View *v, unsigned map_id, int x, int y, int facing);

/* Party state used by the view. */
void mm3_view_set_party(Mm3View *v, int x, int y, int facing);
int mm3_view_x(const Mm3View *v);
int mm3_view_y(const Mm3View *v);
int mm3_view_facing(const Mm3View *v);
void mm3_view_toggle_animation(Mm3View *v); /* the original flips this once per step / turn (byte_28875) */
const Mm3Page *mm3_view_page(const Mm3View *v);

/* Render one frame into a 320x200 8-bit surface (palette indices). */
void mm3_view_render(Mm3View *v, uint8_t *screen);
const Mm3Palette *mm3_view_palette(const Mm3View *v);

/* facing helpers: 0 = north (+y), 1 = south (-y), 2 = east (+x), 3 = west (-x) (exploreLoop's turn and step tables) */
int mm3_facing_left(int facing);
int mm3_facing_right(int facing);
void mm3_facing_step(int facing, int *dx, int *dy);
int mm3_facing_wall_side(int facing); /* MM3_SIDE_* of the wall a party member faces */

#endif
