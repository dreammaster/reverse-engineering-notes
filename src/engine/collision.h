/*
 * collision.h - the game's box collision test (CAD.EXE sub_116E0 + exit lookup sub_11968).
 * Verified against the original routine running in the emulator: tools/collision.py, tools/verify_collision.py;
 * docs/formats.md 6.12.  All coordinates are in 1/8 cell units, 8-bit and signed (the original wraps at 8 bits).
 */
#ifndef CADAVER_COLLISION_H
#define CADAVER_COLLISION_H

#include <stdint.h>

typedef struct {
	int8_t x1, y1, x0, y0, zt, zb;      /* the object's box (entry bytes +0..+5) */
	int solid;                          /* entry[14h] < 0, or the position record flag 04h */
	int id;                             /* caller's handle (index of the entry / thing id) */
} ColBox;

typedef struct {
	int8_t x1, y1, x0, y0;              /* door rectangle (ds:5E4..5F3 groups of 4) */
} ColRect;

typedef struct {
	int w8, h8;                         /* ds:5F4, ds:5F5: room width / height * 8 */
	int wallx, wally;                   /* ds:5DF, ds:5E0: wall corner limits */
	int thick_x, thick_y;               /* ds:5D2 (min x0), ds:5D1 (min y0) */
	int nzones;                         /* number of active door rectangles (0..4); zones[0] = ds:5F0, zones[1] = ds:5EC ... */
	ColRect zones[4];
	const ColBox *objs;                 /* the 50-byte object entries (empty ones are skipped by the caller) */
	int nobjs;
	const ColBox *exits;                /* exit entries (ss:[3A]); id = the exit's target object / room */
	int nexits;                         /* the game scans nexits+1 slots, the last being its FFFF terminator */
	int dirbits;                        /* ds:5F9 & 3: non-zero = player moves along x (changes the exit test) */
} ColWorld;

#define COL_MAX 64

typedef struct {
	int zf;                             /* 1 = nothing overlapped */
	int count;                          /* number of overlapped objects, or 0xFF blocked by the room, 0xFE exit found */
	int flag;                           /* 0xFD when a solid object was hit */
	int n;                              /* entries in ids[] */
	int ids[COL_MAX];
	int exit_id;                        /* when count == 0xFE */
} ColResult;

/* (x1, y1) = candidate position, z = base height, (xs, ys, zs) = box size of the mover, self = id of the mover's own
 * entry (skipped), is_player = the mover's position record has [+4] == 0 (enables the direction rule for exits). */
void collision_test(const ColWorld *w, int x1, int y1, int z, int xs, int ys, int zs, int self, int is_player, ColResult *r);

#endif
