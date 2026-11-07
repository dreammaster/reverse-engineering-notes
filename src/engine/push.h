/*
 * push.h - the player pushing objects (docs/formats.md 6.12, tools/push.py; model validated against the running game).
 */
#ifndef CADAVER_PUSH_H
#define CADAVER_PUSH_H

#include "collision.h"

typedef struct {
	int x1, y1, z0;         /* position record of the object */
	int xs, ys, zs;         /* box size (sprite header words 8..10) */
	int weight;             /* sprite header byte 13 */
} PushObj;

#define PUSH_MAX 64
enum { PUSH_FREE, PUSH_MOVED, PUSH_BLOCKED, PUSH_SOLID, PUSH_FAIL };

/* objs[i] belongs to w->objs[i] (same order, same id). The mover is at (px, py, pz) with box size (pxs, pys, pzs); the move is (dx, dy).
 * On PUSH_MOVED / PUSH_FREE, moved[] receives the indices of the objects that have to move by (dx, dy) (the caller moves them and the
 * mover). Returns a PUSH_* status. strength = 50 in the original (ds:59C). */
int push_move(const ColWorld *w, const PushObj *objs, int px, int py, int pz, int pxs, int pys, int pzs, int pself, int dx, int dy,
              int strength, int moved[], int *nmoved);

#endif
