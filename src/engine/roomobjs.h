/*
 * roomobjs.h - placing the objects ("things") of a room on screen.
 * Verified against the running game: docs/formats.md 6.12, tools/objplace.py.
 */
#ifndef CADAVER_ROOMOBJS_H
#define CADAVER_ROOMOBJS_H

#include <stdint.h>
#include "mp2.h"
#include "objsprites.h"
#include "anim.h"

/* Screen position (top-left pixel of the sprite) of an object at (x1, y1, z0) whose sprite block header is `hdr`
 * (32 bytes as stored in the file, big-endian words), for a room whose tile (0,0) starts at pixel (gx, gy). */
void object_project(int x1, int y1, int z0, const uint8_t hdr[32], int gx, int gy, int *sx, int *sy);

/* The collision box of the object entry: x1, y1, x0, y0, zt, zb after the trimming of CAD.EXE sub_14CB9 (header bytes 0Eh, 1Ah, 1Bh;
 * low nibble shrinks the far side, high nibble the near side). */
void object_trimmed_box(int x1, int y1, int z0, const uint8_t hdr[32], int8_t out[6]);

/* Box occupied by the object: x0..x1, y0..y1, z0..zt (inclusive, 1/8 cell units). */
void object_box(int x1, int y1, int z0, const uint8_t hdr[32], int *x0, int *y0, int *zt);

/* Draws every object of `room` (thing list = MP2 section 2) into the 320x136 canvas (colour 0 = transparent;
 * objects flagged "not in the room" (+3 & 0x80) and blocks that are not plain sprites are skipped).
 * Objects are drawn in list order with the first frame of their animation script (the real draw order and the animation timing are not decoded).
 * Returns the number of objects drawn. */
int room_render_objects(const Mp2 *mp2, const ObjSprites *objs, int room, int gx, int gy, uint8_t *canvas);

/* Animated blocks: per-frame (dx, dy) offsets from the projected position (signed pixels) and the first frame of the script. */
int objsprite_frame_offset(const ObjSprite *sp, int k, int *dx, int *dy);
int objsprite_initial_frame(const ObjSprite *sp);

#define ROOMOBJS_MAX 512
typedef struct {
	int thing, x1, y1, z0;
	int x0, y0, zt;             /* untrimmed box: x0..x1, y0..y1, z0..zt */
	int cx1, cy1, cx0, cy0, czt, czb;   /* collision box kept by the game (trimmed by header bytes 0Eh, 1Ah, 1Bh) */
	int solid;                  /* sprite header flag (file byte 12) bit 0 clear: blocks movement */
	const ObjSprite *sp;
	int sx, sy;                 /* projected top-left of the sprite */
	int animated, first;
	AnimState an;
} RoomObj;
typedef struct {
	int n;
	RoomObj o[ROOMOBJS_MAX];
} RoomObjs;

/* Runtime objects of a room: build once on entry, step every game step (2 timer ticks), draw every frame.
 * use_state = 0 draws the first script frame of animated objects (static, used by the unit tests). */
int roomobjs_build(RoomObjs *ro, const Mp2 *mp2, const ObjSprites *objs, int room, int gx, int gy);
void roomobjs_step(RoomObjs *ro);
void roomobjs_draw(const RoomObjs *ro, uint8_t *canvas, int use_state);
/* Same, but only the objects that are behind (front = 0) or in front of (front = 1) a moving box (px1, py1 = far corner, pz1 = top,
 * pz0 = base): an object is in front when it starts beyond the box along x or y, or above it. Approximation of the game's
 * depth ordering, which is not decoded. */
void roomobjs_draw_rel(const RoomObjs *ro, uint8_t *canvas, int use_state, int px1, int py1, int pz0, int pz1, int front);

/* Raw 32-byte header of a block of section 2 (also for blocks that are not plain sprites, e.g. the player's block 8). */
const uint8_t *objsprites_header(const ObjSprites *o, int block);

const ObjSprite *objsprites_find(const ObjSprites *o, int block);

#endif
