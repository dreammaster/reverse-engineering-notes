/*
 * tad.h - the native routines of ROUTINEn.TAD run on the 8086 interpreter (engine/x86.c), docs/formats.md 6.7.
 * The level's code blob is loaded as a code segment; the routines work on a 64 KB image of the game's data segment (script globals ds:590..71F,
 * event queue) and call engine functions through the service table at ss:[ds:B6]. Services implemented so far: random (8840), energy (81C8),
 * sound (164E, ignored); the others are counted in `unsupported` and return.
 */
#ifndef CADAVER_TAD_H
#define CADAVER_TAD_H

#include <stddef.h>
#include <stdint.h>
#include "acl.h"
#include "x86.h"

#define TAD_TIMERS 16

typedef struct {
	uint8_t *mem;                 /* 1 MB */
	X86 cpu;
	int loaded;
	size_t len;
	int ntimers;                  /* entries in the timer routine table (blob offset from the word at +0x2E... see tad.c) */
	uint16_t timer_entry[TAD_TIMERS];
	int nbeh;                     /* behaviour routines (word at table + 4*k, k >= 9, + 2) */
	uint16_t beh_entry[16];
	unsigned base;                /* data table offset in the blob */
	int unsupported;              /* service calls that were skipped */
	uint16_t last_unsupported;
	AclVM *vm;                    /* state the services act on while a routine runs */
} TadRt;

/* `raw` = unpacked ROUTINEn.TAD. Returns 0 on success. */
int tad_load(TadRt *t, const uint8_t *raw, size_t len);
void tad_free(TadRt *t);
/* Run timer routine `idx` (the countdown with that index expired). Script globals are copied from/to vm->g; events the routine queued are applied
 * (type 8 = show string id as a message). Returns 0, or -1 if the routine failed. */
int tad_run_timer(TadRt *t, AclVM *vm, int idx);
/* Run behaviour routine number b (the routine of table index b + 9) for a creature at (cx, cy) with the player at (px, py). The data-segment image gets a
 * player record, a creature record and its movement state (RAM layout, tools/verify_x86.py); the routine writes the velocity into state bytes 3 and 4.
 * Returns 0 and the signed velocity, or -1. `flags` = ds:650/651/6D7 style switches (0 = deterministic chase). */
int tad_run_behaviour(TadRt *t, AclVM *vm, int b, int cx, int cy, int px, int py, int *vx, int *vy);
/* One 17-step timer unit of 1000:11EB5: counts the active countdowns (vars[0x1A + i], count in vars[0x19]) down and runs the routines of those that reach 0. */
void tad_timer_unit(TadRt *t, AclVM *vm);

#endif
