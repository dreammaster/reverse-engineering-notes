#include "mm2_time.h"
#include "mm2_party.h"
#include "mm2_tables.h"

static uint8_t *P(Mm2Roster *r, unsigned dg) { return mm2_state_ptr((Mm2State *)r->state, dg); }

static unsigned rd16(const Mm2Roster *r, unsigned dg) {
	const uint8_t *p = mm2_state_ptr((Mm2State *)r->state, dg);
	return (unsigned)(p[0] | (p[1] << 8));
}

static void wr16(Mm2Roster *r, unsigned dg, unsigned v) {
	uint8_t *p = P(r, dg);
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
}

int mm2_era(const Mm2Roster *r) { return (int)rd16(r, 0x3CA); }
int mm2_day_of_year(const Mm2Roster *r) { return (int)rd16(r, 0x3A2u + 2u * (unsigned)mm2_era(r)); }
int mm2_year(const Mm2Roster *r) { return (int)rd16(r, 0x3B6u + 2u * (unsigned)mm2_era(r)); }
int mm2_day_fraction(const Mm2Roster *r) { return (int)rd16(r, 0x3CC); }

static void age_party_one_day(Mm2Roster *r) {
	int i;
	for (i = 0; i < mm2_party_size(r); i++) {
		Mm2Char *c = &r->chars[mm2_party_member(r, i)];
		c->raw[0x22]++;
		if (c->raw[0x22] >= 0xB5) {   /* 181 days: a year older */
			c->raw[0x21]++;
			c->raw[0x22] = 1;
		}
	}
}

/* TODO(review): transcribed from resident advance_time (mm2.asm IDA 0x150CE..0x15184).  The redraw side effects are dropped; the
 * flags cleared on days 60/120/180 are byte_1DC44/45 (DGROUP:03F4/03F5, in the timers block of the saved state) and
 * byte_1DC3A (DGROUP:03EA) is cleared at the year roll; their meaning is unknown. */
void mm2_advance_time(Mm2Roster *r, int units, int darkCell) {
	unsigned frac, era = (unsigned)mm2_era(r), dayDg = 0x3A2u + 2u * era, yearDg = 0x3B6u + 2u * era;
	if (units == 1 && darkCell) {
		uint8_t *light = P(r, 0x3D5);
		if (*light) (*light)--;
	}
	frac = rd16(r, 0x3CC) + (unsigned)units;
	wr16(r, 0x3CC, frac);
	if (frac < 0x100) return;
	wr16(r, dayDg, rd16(r, dayDg) + 1);
	wr16(r, 0x3CC, frac % 0x100);
	age_party_one_day(r);
	{
		unsigned d = rd16(r, dayDg);
		if (d == 0x3C || d == 0x78 || d == 0xB4) {
			*P(r, 0x3F4) = 0;
			*P(r, 0x3F5) = 0;
		}
		if (d > 0xB4) {
			wr16(r, dayDg, 1);
			if (rd16(r, yearDg) != 0x3E7) wr16(r, yearDg, rd16(r, yearDg) + 1);
			*P(r, 0x3EA) = 0;
		}
	}
}

static int bracket_or_zero(int v) {
	int b = mm2_bracket(v);
	return (uint8_t)b >= 0xF2 ? 0 : b;
}

/* TODO(review): transcribed from ovl/2MISC.asm party_do_rest (IDA 0x1CD8A..0x1CEDB).  Not ported: the hireling upkeep
 * (party_pay_hireling_upkeep), the redraw, and the move to a new map when thrown into era 9 (enter_map is called with FFh,FFh;
 * here only g_era changes).  The effect bytes cleared are 1DC25..1DC2F + 1DC30/31 (DGROUP:03D5..03E1). */
int mm2_party_rest(Mm2Roster *r, const Mm2Rng *rng) {
	static const unsigned FX[] = {0x3DA, 0x3D9, 0x3D8, 0x3D7, 0x3D6, 0x3D5, 0x3E1, 0x3E0, 0x3DB, 0x3DF, 0x3DE, 0x3DD, 0x3DC};
	unsigned i;
	int m, jumped = 0;
	for (i = 0; i < sizeof(FX) / sizeof(FX[0]); i++)
		*P(r, FX[i]) = 0;
	for (m = 0; m < mm2_party_size(r); m++) {
		Mm2Char *c = &r->chars[mm2_party_member(r, m)];
		uint8_t *x = c->raw;
		if (x[MC_CONDITION] >= 0x80) continue;
		x[MC_CONDITION] &= 0x0D;
		if (x[MC_AGE] >= 0x50 && rng->range(rng->ud, 1, 100) < 50) x[MC_CONDITION] = 0x81;
		if (x[MC_HP] == 0 && x[MC_HP + 1] == 0) x[MC_HP] = 1;
		if (x[MC_CONDITION] & 0x08) {   /* poisoned: the maximum is halved */
			unsigned mx = mm2_c16(c, MC_HP_MAX) >> 1;
			x[MC_HP_MAX] = (uint8_t)mx;
			x[MC_HP_MAX + 1] = (uint8_t)(mx >> 8);
		} else {
			x[MC_HP_MAX] = x[0x60];
			x[MC_HP_MAX + 1] = x[0x61];
		}
		if (x[MC_FOOD] == 0) continue;
		x[MC_FOOD]--;
		if (!(x[MC_CONDITION] & 0x04)) {
			x[MC_HP] = x[MC_HP_MAX];
			x[MC_HP + 1] = x[MC_HP_MAX + 1];
		}
		if (x[MC_BASE_SPELL_LEVEL]) {
			int cls = x[MC_CLASS];
			int stat = x[(cls == MM2_SORCERER || cls == MM2_ARCHER) ? MC_BASE_STATS + 1 : MC_BASE_STATS + 2];
			unsigned sp = (unsigned)x[MC_BASE_LEVEL] * (unsigned)(bracket_or_zero(stat) + 3);
			x[MC_SP_MAX] = (uint8_t)sp;
			x[MC_SP_MAX + 1] = (uint8_t)(sp >> 8);
		}
		/* char_reset_current_stats (mm2.asm 0x13572) */
		mm2_char_reset_current_stats(c);
		x[MC_ENDURANCE] = x[MC_BASE_ENDURANCE];
		x[0x70] = x[0x15];
		x[MC_SP] = x[MC_SP_MAX];
		x[MC_SP + 1] = x[MC_SP_MAX + 1];
	}
	mm2_advance_time(r, 85, 0);
	if (mm2_era(r) != 9 && rng->range(rng->ud, 1, 60) < 10) {
		wr16(r, 0x3CA, 9);
		jumped = 1;
	}
	return jumped;
}
