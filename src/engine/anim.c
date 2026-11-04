#include "anim.h"

int anim_script_len(const ObjSprite *sp) {
	if (sp->extra_len < 8)
		return 0;
	int sl = (sp->extra[2] << 8) | sp->extra[3];
	if (8 + sl > sp->extra_len)
		return 0;
	return sl / 2;
}

void anim_script_word(const ObjSprite *sp, int i, int *cmd, int *arg) {
	*cmd = sp->extra[8 + 2 * i];
	*arg = sp->extra[8 + 2 * i + 1];
}

void anim_step(AnimState *st, const ObjSprite *sp, int *events) {
	int n = anim_script_len(sp);
	if (n == 0)
		return;
	int pos = st->pos, cmd, arg;
	for (int guard = 0; guard < 4 * n + 8; guard++) {
		if (pos < 0 || pos >= n)
			pos = 0;
		if (st->delay != 0)
			break;
		anim_script_word(sp, pos, &cmd, &arg);
		if (cmd == 0)
			break;
		int idx = 0xFF - cmd, stop = 0;
		switch (idx) {
		case 0: st->delay = 0xFF; stop = 1; break;                       /* FF freeze */
		case 1: st->delay = arg; stop = 1; break;                        /* FE wait arg steps */
		case 2: pos = 0; break;                                          /* FD loop */
		case 3: pos = arg; break;                                        /* FC goto */
		case 4: st->dir = 1; pos -= 2; break;                            /* FB backwards */
		case 5: if (st->dir) pos++; pos++; st->dir = 0; break;           /* FA turn round */
		case 6: st->delay = 0xFE; if (events) (*events)++; stop = 1; break; /* F9 freeze + trigger */
		default: if (events) (*events)++; pos++; break;                  /* F8.. events / counters */
		}
		pos &= 0xFF;
		st->pos = pos;
		if (stop)
			break;
	}
	if (pos < 0 || pos >= n)
		pos = 0;
	int advance = 0;
	if (st->delay == 0) {
		anim_script_word(sp, pos, &cmd, &arg);
		st->frame = arg;
		advance = 1;
	} else if (st->delay < 0x80) {
		st->delay--;
		advance = st->delay == 0;
	}
	if (advance) {
		if (st->dir == 0) {
			pos++;
		} else {
			pos--;
			if (pos == 0)
				st->dir = 0;
		}
		pos &= 0xFF;
		if (st->cad == 0) {
			st->pos = pos;
		} else if (--st->cnt < 0) {
			st->cnt = st->cad;
			st->pos = pos;
		}
	}
	st->dx = st->dy = 0;
	if (st->frame >= 0 && st->frame < sp->nframes) {
		int sl = (sp->extra[2] << 8) | sp->extra[3], o = 8 + sl + 8 * st->frame + 2;
		if (o + 4 <= sp->extra_len) {
			st->dx = (int16_t)((sp->extra[o] << 8) | sp->extra[o + 1]);
			st->dy = (int16_t)((sp->extra[o + 2] << 8) | sp->extra[o + 3]);
		}
	}
}
