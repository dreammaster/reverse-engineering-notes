#include "mm2_inn.h"

static Mm2State *st(Mm2Roster *r) { return (Mm2State *)r->state; }
static const Mm2State *cst(const Mm2Roster *r) { return (const Mm2State *)r->state; }

int mm2_party_size(const Mm2Roster *r) {
	return (int)mm2_state_party_size(cst(r));
}

int mm2_party_member(const Mm2Roster *r, int slot) {
	unsigned id = mm2_state_party_id(cst(r), slot);
	return id == 0xFFFF || slot >= mm2_party_size(r) ? -1 : (int)id;
}

static void set_slot(Mm2Roster *r, int slot, unsigned id) {
	uint8_t *p = mm2_state_ptr(st(r), 0x416u + 2u * (unsigned)slot);
	p[0] = (uint8_t)id;
	p[1] = (uint8_t)(id >> 8);
}

static void set_size(Mm2Roster *r, int n) {
	uint8_t *p = mm2_state_ptr(st(r), 0x426);
	p[0] = (uint8_t)n;
	p[1] = 0;
}

/* TODO(review): ovl/1RETINN.asm inn_menu (IDA 0x1C5C0..0x1CB3F).  The rule "a hireling is offered only when event variable
 * (id - 24) is set" comes from my reading of the code that tests DGROUP:3F6 + index; the exact index mapping of hireling
 * ids to variables was not verified for all 24 hirelings. */
static int hireling_unlocked(const Mm2Roster *r, int id) {
	unsigned dg = mm2_event_var_dgroup(id - MM2_FIRST_HIRELING);
	uint8_t *p = dg ? mm2_state_ptr((Mm2State *)cst(r), dg) : 0;
	return p && *p;
}

int mm2_inn_list(const Mm2Roster *r, int town, int out[MM2_ROSTER_CHARS]) {
	int i, n = 0;
	for (i = 0; i < MM2_ROSTER_CHARS; i++) {
		const Mm2Char *c = &r->chars[i];
		if (!c->raw[MC_NAME] || c->raw[MC_TOWN] != town + 1) continue;
		if (i >= MM2_FIRST_HIRELING && !hireling_unlocked(r, i)) continue;
		out[n++] = i;
	}
	return n;
}

static int in_party(const Mm2Roster *r, int id) {
	int i;
	for (i = 0; i < mm2_party_size(r); i++)
		if (mm2_party_member(r, i) == id) return i;
	return -1;
}

Mm2InnResult mm2_inn_add(Mm2Roster *r, int id) {
	int i, chars = 0, hires = 0, n = mm2_party_size(r);
	if (id < 0 || id >= MM2_ROSTER_CHARS || !r->chars[id].raw[MC_NAME]) return MM2_INN_NOT_FOUND;
	if (id >= MM2_FIRST_HIRELING && !hireling_unlocked(r, id)) return MM2_INN_UNAVAILABLE;
	if (in_party(r, id) >= 0) return MM2_INN_ALREADY;
	for (i = 0; i < n; i++) {
		if (mm2_party_member(r, i) >= MM2_FIRST_HIRELING) hires++;
		else chars++;
	}
	if (id >= MM2_FIRST_HIRELING ? hires >= MM2_MAX_HIRELINGS : chars >= MM2_MAX_CHARS_IN_PARTY) return MM2_INN_FULL;
	set_slot(r, n, (unsigned)id);
	set_size(r, n + 1);
	return MM2_INN_OK;
}

Mm2InnResult mm2_inn_remove(Mm2Roster *r, int id) {
	int slot = in_party(r, id), i, n = mm2_party_size(r);
	if (slot < 0) return MM2_INN_NOT_FOUND;
	for (i = slot; i < n - 1; i++)
		set_slot(r, i, (unsigned)mm2_party_member(r, i + 1));
	set_slot(r, n - 1, 0xFFFF);
	set_size(r, n - 1);
	return MM2_INN_OK;
}

Mm2InnResult mm2_inn_move(Mm2Roster *r, int id, int town) {
	if (id < 0 || id >= MM2_ROSTER_CHARS || !r->chars[id].raw[MC_NAME]) return MM2_INN_NOT_FOUND;
	if (in_party(r, id) >= 0) return MM2_INN_ALREADY;
	r->chars[id].raw[MC_TOWN] = (uint8_t)(town + 1);
	return MM2_INN_OK;
}

/* TODO(review): ovl/1RETINN.asm inn_menu exit path (IDA ~0x1CB15) also stamps g_inn_town and saves the roster; the party
 * members' town byte update (+0Bh) is from docs/party-commands.md, not re-checked. */
void mm2_inn_leave(Mm2Roster *r, int town) {
	int i;
	for (i = 0; i < mm2_party_size(r); i++)
		r->chars[mm2_party_member(r, i)].raw[MC_TOWN] = (uint8_t)(town + 1);
}
