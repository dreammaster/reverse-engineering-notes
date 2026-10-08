/* The translated 3D view pipeline (prepareIndoorView + renderIndoorView and the routines under them). */
#ifndef MM3_VIEW_HOST_H
#define MM3_VIEW_HOST_H

#include "recomp.h"

typedef void (*MmViewListFn)(void *user, unsigned list_addr);

/* The game's data segment: callers fill DG[] (party position, maze page slots, view state) and read the draw lists back from it. */
void mm3_view_set_list_callback(MmViewListFn fn, void *user);
void mm3_view_run(void); /* run prepareIndoorView and renderIndoorView on DG[]; draw lists are reported through the callback */

/* Map change callbacks: mazeUpdateSlot (translated) asks the glue to reload pages / graphics / the map's monsters when the party
 * crosses into another page of an outdoor map.  Arguments are as in the original (map id - 1). */
typedef struct {
	void *user;
	void (*load_map_data)(void *user, unsigned map_minus_1);
	void (*load_map_graphics)(void *user, unsigned map_minus_1);
	void (*map_load)(void *user, unsigned map_minus_1);
} MmViewMapOps;
void mm3_view_set_map_ops(const MmViewMapOps *ops);

void mm3_view_run_outdoor(void);     /* drawViewOutdoors (maps with Maze_wrapMode set) */
void mm3_view_update_slot(void);     /* mazeUpdateSlot: after the party moved */

#endif
