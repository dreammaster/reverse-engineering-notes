/* The translated 3D view pipeline (prepareIndoorView + renderIndoorView and the routines under them). */
#ifndef MM3_VIEW_HOST_H
#define MM3_VIEW_HOST_H

#include "recomp.h"

typedef void (*MmViewListFn)(void *user, unsigned list_addr);

/* The game's data segment: callers fill DG[] (party position, maze page slots, view state) and read the draw lists back from it. */
void mm3_view_set_list_callback(MmViewListFn fn, void *user);
void mm3_view_run(void); /* run prepareIndoorView and renderIndoorView on DG[]; draw lists are reported through the callback */

#endif
