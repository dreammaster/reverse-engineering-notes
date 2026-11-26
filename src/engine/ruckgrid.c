#include "ruckgrid.h"

void ruckgrid_nav(int key, int n, int *gx, int *grow, int *gvis, int *gscroll) {
	int R = n > 1 ? (n - 1) / 4 : 0, C = n > 0 ? (n - 1) % 4 : 0;
	int dl = *gx, dh = *grow, cl = *gvis, bh = *gscroll, clamp = 0;
	if (key & 1) {                                                    /* up (loc_12790) */
		int skip = 0;
		if (--cl < 0) {
			if (R <= 3) { cl = R; dh = R; clamp = 1; skip = 1; }
			else { cl = 0; if (--bh < 0) bh = R; }
		}
		if (!skip) { if (--dh < 0) { dh = R; clamp = 1; } }
	} else if (key & 2) {                                             /* down (loc_127B7) */
		int wrap = 0;
		if (++cl == 4) {
			if (R == 3) wrap = 1;
			else { cl = 3; if (++bh > R) bh = 0; }
		}
		if (!wrap && R >= cl) {
			if (++dh == R) clamp = 1;
			else if (dh > R) dh = 0;
		} else { dh = 0; cl = 0; clamp = 1; }                          /* loc_127DA */
	} else if (key & 4) {                                             /* left (loc_127F8) */
		if (--dl < 0) dl = dh == R ? C : 3;
	} else if (key & 8) {                                             /* right (loc_1280C) */
		dl++;
		if (dh == R) { if (dl > C) dl = 0; }
		else if (dl >= 4) dl = 0;
	}
	if (clamp && dl >= C) dl = C;
	*gx = dl; *grow = dh; *gvis = cl; *gscroll = bh;
}
