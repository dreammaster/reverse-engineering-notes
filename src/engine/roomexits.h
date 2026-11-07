/*
 * roomexits.h - door boxes ("exits") of a room, as built by CAD.EXE 1000:4D00..4ECD (verified against the original for all 72
 * rooms of level 1, tools/roomexits.py, docs/formats.md 6.12).
 */
#ifndef CADAVER_ROOMEXITS_H
#define CADAVER_ROOMEXITS_H

#include "mp2.h"
#include "objsprites.h"
#include "collision.h"
#include "roomobjs.h"

#define ROOM_MAX_EXITS 7

/* Fills out[] (id = door id, i.e. the MP2 section 1 record number) in room-record order. Returns the number of exits. */
int room_build_exits(const Mp2 *mp2, const ObjSprites *objs, int room, ColBox out[ROOM_MAX_EXITS]);

/* Where the player appears in `room` after coming through the door whose box in that room is `e` (tools/landing.py, verified). */
void room_landing(const Mp2 *mp2, int room, const ColBox *e, int *x1, int *y1);

#endif
