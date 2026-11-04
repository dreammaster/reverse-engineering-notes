/*
 * roomobjs.h - placing the objects ("things") of a room on screen.
 * Verified against the running game: docs/formats.md 6.12, tools/objplace.py.
 */
#ifndef CADAVER_ROOMOBJS_H
#define CADAVER_ROOMOBJS_H

#include <stdint.h>
#include "mp2.h"
#include "objsprites.h"

/* Screen position (top-left pixel of the sprite) of an object at (x1, y1, z0) whose sprite block header is `hdr`
 * (32 bytes as stored in the file, big-endian words), for a room whose tile (0,0) starts at pixel (gx, gy). */
void object_project(int x1, int y1, int z0, const uint8_t hdr[32], int gx, int gy, int *sx, int *sy);

/* Box occupied by the object: x0..x1, y0..y1, z0..zt (inclusive, 1/8 cell units). */
void object_box(int x1, int y1, int z0, const uint8_t hdr[32], int *x0, int *y0, int *zt);

/* Draws every object of `room` (thing list = MP2 section 2) into the 320x136 canvas (colour 0 = transparent;
 * objects flagged "not in the room" (+3 & 0x80) and blocks that are not plain sprites are skipped).
 * Objects are drawn in list order with frame 0 (the real draw order and frame selection are not decoded).
 * Returns the number of objects drawn. */
int room_render_objects(const Mp2 *mp2, const ObjSprites *objs, int room, int gx, int gy, uint8_t *canvas);

const ObjSprite *objsprites_find(const ObjSprites *o, int block);

#endif
