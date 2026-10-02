#include "mm2_game.h"
#include "mm2_map.h"
#include "mm2_view.h"

#include <stdlib.h>
#include <string.h>

static int session_rand(void *ud, int lo, int hi) {
	(void)ud;
	return hi <= lo ? lo : lo + rand() % (hi - lo + 1);
}

int mm2_session_message(const Mm2GameSession *s, int n, char *out, size_t cap) {
	const uint8_t *p = s->events.messages, *end = p + s->events.messagesLen;
	size_t len = 0;
	if (!s->hasEvents || n < 1 || cap == 0) return -1;
	while (--n > 0) {
		while (p < end && *p != 0xFF)
			p++;
		if (p >= end) return -1;
		p++;
	}
	while (p < end && *p != 0xFF) {
		int c = *p++ & 0x7F;
		if (len + 1 < cap) out[len++] = c == 0x40 ? '\n' : (char)c;
	}
	out[len] = 0;
	return (int)len;
}

const char *mm2_location_name(int c) {
	switch (c) {
	case 1: return "Inn";
	case 2: return "Training hall";
	case 3: case 7: case 8: return "Tavern";
	case 4: return "Temple";
	case 5: return "Mage guild";
	case 6: return "Blacksmith";
	case 0x64: return "Quest";
	case 0x7E: return "Slide";
	case 0x7F: return "Ambush";
	case 0x80: return "Teleport trap";
	case 0x81: case 0x82: case 0x83: return "Found item";
	case 0xC9: return "Lord Hoardall";
	case 0xCA: return "Lord Slayer";
	case 0xCB: case 0xCC: case 0xCD: case 0xCE: return "Donation hall";
	case 0xCF: return "Time travel";
	case 0xE2: return "Town crier";
	case 0xFD: return "Blacksmith robbery";
	default: return "Entrance";
	}
}

/* TODO(review): simplified host for the events: ask_yn (ops 9/10, ovl/2PLAY.asm 0x1941E/0x1946E) just returns the preset
 * answer, teleports end the script, fights only record the monster ids (op 18 args read as 10 ids + 2 parameters, 0x19912);
 * message ops ignore their different window styles. */
static int host_exec(void *ud, Mm2Vm *vm, int op, const uint8_t *args) {
	Mm2GameSession *s = (Mm2GameSession *)ud;
	switch (op) {
	case EV_MSG: case EV_MSG_ROW13: case EV_MSG_FRAME: case EV_TITLE: case EV_MSG_BOX: case EV_MSG_FRAMED:
		if (s->nMessages < MM2_GAME_MAX_MSGS) {
			Mm2GameMessage *m = &s->messages[s->nMessages++];
			m->opcode = op;
			if (mm2_session_message(s, args[0], m->text, sizeof(m->text)) < 0) m->text[0] = 0;
		}
		break;
	case EV_LOCATION:
		if (s->nLocations < MM2_GAME_MAX_MSGS) s->locations[s->nLocations++] = args[0];
		break;
	case EV_TELEPORT:
		s->pendingMap = args[0] & 0x7F;
		s->pendingX = args[1] & 15;
		s->pendingY = args[1] >> 4;
		return 0;   /* the party leaves the script's cell */
	case EV_FIGHT:
		s->fightRequested = 1;
		memcpy(s->fightMonsters, args, 10);
		break;
	case EV_FIGHT2:
		s->fightRequested = 1;
		memcpy(s->fightMonsters, args, 10);
		break;
	case EV_ASK_YN:
	case EV_ASK_YN2:
		vm->cond = s->yesNo;
		break;
	case EV_CLEAR_TRIGGER:   /* one-shot events: clear the cell's trigger flag (bit 7) */
		s->data[256 + ((s->y & 15) << 4 | (s->x & 15))] &= 0x7F;
		break;
	case EV_PLACE_TREASURE: {   /* gold (3 bytes), gems (word), then 3 x {item id, charges, flags}; docs/events.md */
		int k;
		memset(&s->treasure, 0, sizeof(s->treasure));
		s->treasure.gold = (uint32_t)(args[0] | (args[1] << 8) | (args[2] << 16));
		s->treasure.gems = args[3] | (args[4] << 8);
		for (k = 0; k < 3; k++) {
			s->treasure.items[k].item = args[5 + 3 * k];
			s->treasure.items[k].charges = args[6 + 3 * k];
			s->treasure.items[k].flags = args[7 + 3 * k];
		}
		s->treasureHere = 1;
		break;
	}
	default:
		break;
	}
	return 1;
}

static int load_map(Mm2GameSession *s, int map) {
	mm2_blob_free(&s->eventBlob);
	s->hasEvents = 0;
	if (!mm2_load_map(s->files, map, s->data)) return 0;
	s->eventBlob = mm2_load_events(s->files, map);
	if (s->eventBlob.data && mm2_parse_events(&s->eventBlob, &s->events)) s->hasEvents = 1;
	s->map = map;
	return 1;
}

int mm2_session_start(Mm2GameSession *s, const Mm2Game *files, int map, int x, int y, char facing) {
	memset(s, 0, sizeof(*s));
	s->files = files;
	s->pendingMap = -1;
	s->yesNo = 1;
	if (!load_map(s, map)) return 0;
	s->x = x;
	s->y = y;
	s->facing = facing;
	s->host.exec = host_exec;
	s->host.rand_range = session_rand;
	s->host.ud = s;
	s->vm.state = &s->state;
	s->vm.host = &s->host;
	return 1;
}

void mm2_session_attach(Mm2GameSession *s, Mm2Roster *roster) {
	s->vm.roster = roster;
	s->vm.state = (Mm2State *)roster->state;
	s->vm.map = s->data;
}

void mm2_session_end(Mm2GameSession *s) {
	mm2_blob_free(&s->eventBlob);
}

void mm2_session_turn(Mm2GameSession *s, int dir) {
	static const char order[] = "NESW";
	int i = (int)(strchr(order, s->facing) - order);
	s->facing = order[(i + (dir > 0 ? 1 : 3)) & 3];
}

int mm2_session_run_trigger(Mm2GameSession *s) {
	int script;
	s->nMessages = s->nLocations = 0;
	s->fightRequested = 0;
	s->pendingMap = -1;
	if (!s->hasEvents) return 0;
	script = mm2_event_find_trigger(&s->events, s->x, s->y, s->facing);
	if (script < 0) return 0;
	mm2_vm_run(&s->vm, s->events.scripts, s->events.scriptsLen, script);
	if (s->pendingMap >= 0) {
		int m = s->pendingMap, x = s->pendingX, y = s->pendingY;
		if (m != s->map && !load_map(s, m)) return 1;
		s->x = x;
		s->y = y;
		s->pendingMap = -1;
	}
	return 1;
}

/* TODO(review): movement/trigger timing is simplified.  The original (resident party_step_forward, mm2.asm IDA 0x142AA, and
 * game_main_loop) also ticks effect timers, may start a random encounter (chance from attribute byte 09), clears spell
 * effects and redraws; the trigger is run with the party's current facing, which I assumed matches evt_check_cell_triggers
 * (ovl/2PLAY.asm 0x1A8C4, facing table DGROUP:16D2). */
int mm2_session_step(Mm2GameSession *s, int backwards) {
	char dir = s->facing;
	int dx, dy;
	if (backwards) {
		mm2_session_turn(s, 1);
		mm2_session_turn(s, 1);
		dir = s->facing;
		mm2_session_turn(s, 1);
		mm2_session_turn(s, 1);
	}
	if (!mm2_can_step(s->data, s->x, s->y, dir)) return 0;
	mm2_facing_delta(dir, &dx, &dy);
	s->x = (s->x + dx) & 15;
	s->y = (s->y + dy) & 15;
	mm2_session_run_trigger(s);
	return 1;
}
