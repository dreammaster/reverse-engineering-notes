#include "party.h"

#include <string.h>

static uint16_t u16(const uint8_t *d, size_t o) { return (uint16_t)(d[o] | (d[o + 1] << 8)); }
static uint32_t u32(const uint8_t *d, size_t o) { return u16(d, o) | ((uint32_t)u16(d, o + 2) << 16); }

int mm3_party_load(Mm3Party *p, const uint8_t *d, size_t len) {
	if (len < MM3_PTY_SIZE)
		return -1;
	memset(p, 0, sizeof(*p));
	memcpy(p->raw, d, MM3_PTY_SIZE);
	p->count = d[0x00];
	memcpy(p->member, d + 1, MM3_MAX_PARTY);
	p->facing = d[0x0A]; p->x = d[0x0B]; p->y = d[0x0C]; p->map = d[0x0D];
	p->sound_fx = d[0x0E]; p->music = d[0x0F]; p->option = d[0x10]; p->last_inn_town = d[0x11];
	p->levitate = d[0x12]; p->wizard_eye = d[0x14]; p->walk_on_water = d[0x15];
	p->day = d[0x34B]; /* 0-99 */
	p->year = u16(d, 0x34C);
	p->light = u16(d, 0x34E);
	p->fire = u16(d, 0x350); p->elec = u16(d, 0x352); p->cold = u16(d, 0x354); p->poison = u16(d, 0x356);
	p->minutes = u16(d, 0x358);
	p->food = u16(d, 0x35A);
	p->bank_gold = u32(d, 0x362); p->bank_gems = u32(d, 0x366);
	p->gold = u32(d, 0x36A); p->gems = u32(d, 0x36E);
	memcpy(p->event_bytes, d + 0x341, sizeof(p->event_bytes));
	memcpy(p->flags, d + 0x376, MM3_FLAG_BYTES);
	return 0;
}

int mm3_party_flag(const Mm3Party *p, unsigned bit) {
	if (bit >= MM3_FLAG_BYTES * 8)
		return 0;
	return (p->flags[bit >> 3] >> (bit & 7)) & 1; /* bit order within a byte: see note in README, unverified */
}

int mm3_roster_load(Mm3Character *out, unsigned max, const uint8_t *d, size_t len) {
	unsigned n = (unsigned)(len / sizeof(Mm3Character));
	if (n > max)
		n = max;
	memcpy(out, d, n * sizeof(Mm3Character)); /* little-endian host assumed (x86/ARM) */
	return (int)n;
}
