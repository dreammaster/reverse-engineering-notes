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
} AnimState;

/* Script words of an animated block: (command, argument) pairs. Returns the number of words (0 = static block). */
int anim_script_len(const ObjSprite *sp);
void anim_script_word(const ObjSprite *sp, int i, int *cmd, int *arg);

/* One game step (every 2 timer ticks). Event commands (F8 and below) are skipped and counted in *events if non-NULL. */
void anim_step(AnimState *st, const ObjSprite *sp, int *events);

#endif
