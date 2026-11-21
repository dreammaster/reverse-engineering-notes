/*
 * anim.h - sprite animation stepper (CAD.EXE sub_13BC5 + command table 1000:3C77), docs/formats.md 6.12, tools/animation.py.
 */
#ifndef CADAVER_ANIM_H
#define CADAVER_ANIM_H

#include "objsprites.h"

typedef struct {
	int delay;          /* 0 = running, 1..127 = wait, >=128 frozen (0xFE after F9, 0xFF after FF) */
	int frame;          /* current frame number */
	int dir;            /* 0 forward, 1 backward */
	int pos;            /* position in the script */
	int cnt, cad;       /* cadence: the position advances every cad+1 steps */
	int dx, dy;         /* pixel offset of the current frame */
	int nev;            /* F8 commands run since the caller last looked: event 13 (1000:3CE7) with these arguments */
	uint8_t ev[4];
	int nact;           /* F7 commands: run the thing's TAD action routine n (1000:3D17) */
	uint8_t act[4];
	int ncnt;           /* F6 / F5: A[8 + n] -= 1 / += 1 (1000:3D61 / 3D4B): (n, delta) pairs for the caller */
	int8_t cntn[4], cntd[4];
} AnimState;

/* Script words of an animated block: (command, argument) pairs. Returns the number of words (0 = static block). */
int anim_script_len(const ObjSprite *sp);
void anim_script_word(const ObjSprite *sp, int i, int *cmd, int *arg);

/* One game step (every 2 timer ticks). Event commands (F8 and below) are skipped and counted in *events if non-NULL. */
void anim_step(AnimState *st, const ObjSprite *sp, int *events);

#endif
