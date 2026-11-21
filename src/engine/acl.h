/*
 * acl.h - the ACL script interpreter (subset), docs/formats.md 6.7, tools/acl.py + tools/acl_vm.py (verified against the original).
 * Implemented: thing flag / variable / condition opcodes; everything else is skipped (operand lengths are known for all 94 opcodes).
 */
#ifndef CADAVER_ACL_H
#define CADAVER_ACL_H

#include <stdint.h>
#include "mp2.h"

#define ACL_THINGS 1000

typedef struct {
	uint8_t present[ACL_THINGS];
	uint8_t flags[ACL_THINGS];      /* rec[3]  */
	uint8_t flags2[ACL_THINGS];     /* rec[15] */
	uint8_t room[ACL_THINGS];       /* rec[10] */
	uint8_t tail[ACL_THINGS][16];   /* first bytes of the thing's A struct (rec[rec[12]..]): chest content, key, flags, trap; [1] behaviour no., [8] [9] state of the TAD action routines */
	uint8_t mtab[ACL_THINGS][48];   /* B + 14: scripted movement entries (move x y z counters / jump / wait ...) */
	uint8_t mtab_len[ACL_THINGS];
	uint8_t mstate[ACL_THINGS][10]; /* movement state B (rec[rec[13]..]): [0] mode (3 = velocity driven, 0 = scripted), [6..8] step sizes, [9] speed gate (low nibble counter, high nibble period) */
	uint8_t pos[ACL_THINGS * 3];    /* rec[0..2]: x1, y1, z0 */
	union {                         /* script globals ds:590..71F (the TAD routines see them as part of the data segment) */
		uint8_t g[0x190];
		struct {
			uint8_t pre[0x81];      /* ds:590..610 */
			uint8_t vars[0x40];     /* ds:611.. */
			uint8_t post[0x190 - 0x81 - 0x40];
		};
	};
	int c605;                       /* condition counter */
	int cur;                        /* current object (thing id) */
	uint16_t doorw[400];            /* word at +2 of every door record (section 1): script-visible lock state (ops 0Ah / 1Bh set it, op 3Dh tests it) */
	int held;                       /* ds:27A: the thing held (op 39h tests it) */
	uint8_t size[ACL_THINGS][3];    /* sprite box size (header words 8..10) of every thing, filled by the front end (op 38h needs it) */
	int clone_ok;                   /* the front end lets spawns of a live thing allocate a copy with its own id (900..999) */
	uint16_t clone_src[ACL_THINGS]; /* for such copies: the template whose record they share (0 = not a copy) */
	int describe;                   /* op 48h (look): thing+1 whose picture goes with the message just queued (0 = none) */
	int room_no;                    /* ds:232 */
	int reg;                        /* ds:704 */
	int disarm;
	int skipped;                    /* number of unsupported opcodes skipped so far */
	const Mp2 *mp2;                 /* the level's records (spawn copies a thing's record) */
	int (*rand_range)(int lo, int hi, void *user);
	void (*sound)(int id, void *user);        /* 1000:164E: start sound `id` (NULL = silent) */
	void (*sound_stop)(int id, void *user);   /* 1000:1613 */
	void *user;
	int px, py, pz;                 /* the player's place (ds:5C record): op 49h with a carried source uses it */
	struct { int id, from, vx, vy, dir; } spawn[4];   /* projectiles fired by TAD routines (service 22h): thing id, the shooter, velocity bytes, direction code */
	int nspawn;
	int act_thing[8], act_arg[8], nact;   /* op 42h: TAD action routines to run (thing, argument) */
	int resume[16], nresume;        /* op 03h: things whose frozen animation (delay FEh) continues */
	int queue_n;                    /* room events queued by ops (op 9 -> type 19) */
	struct { int type, arg; } queue[16];
	uint32_t money, score;          /* ds:248 / ds:24C */
	int cap;                        /* ds:5A0 */
	int energy, energy_max;         /* ds:23A / ds:700 */
	struct { int pending, room, x, y, z; } go;   /* op 37 (change room): destination, applied by the caller */
	int next_level;                 /* op 51: level to load next (0 = none) */
	struct { int thing, type; } tev[8];   /* thing events queued by ops (op 50 -> event 23) */
	int tev_n, tev_depth;
	int msg_n;                      /* op 28 (say): string ids waiting to be shown */
	int msg[16];
	int timer_arg, timer_delay;     /* one-shot room timer (op 42): delay in timer units, -1 = none */
	uint8_t armed[128 + ACL_THINGS][64 / 8];   /* per record (rooms first, then things) and entry: 1 = disarmed */
} AclVM;

void acl_init(AclVM *vm, const Mp2 *mp2);
/* re-point the record slots of the copies after the level (and its Mp2) was reloaded */
void acl_restore_clones(AclVM *vm);
/* put a fresh copy of thing `id` at the place of thing `from` (a shot fired by a TAD routine) with the velocity bytes vx, vy */
void acl_spawn_shot(AclVM *vm, int id, int from, int vx, int vy);
/* 1000:81C8 with its sounds */
void acl_energy_change(AclVM *vm, int delta);
/* run all armed handlers of `event` in the handler list of a room record (kind 0) */
void acl_fire_room(AclVM *vm, const Mp2 *mp2, int room, int event);
/* deliver one room event (type 6 enter, 14 timer, 19/20/24 with a byte argument, 28 first visit ...) to the room's handler list and everything the
 * handlers queue, like 1000:7634. Handlers whose condition operand differs from the argument's low byte are skipped. */
void acl_room_event(AclVM *vm, const Mp2 *mp2, int room, int type, int arg);
/* deliver a thing event (7 = touched, 9 = contact with the thing id `arg`, ...) to the thing's handler list; vm->cur = thing. Events queued by ops
 * are left in vm->queue for the caller (they belong to the room's list). */
void acl_thing_event(AclVM *vm, const Mp2 *mp2, int thing, int type, int arg);
/* Verb 3 on a chest (sprite header 0x16 == 8, 1000:31A3): trap damage, key check, content, open bit. Returns 1 when the chest changed state;
 * *content = thing id to place into the room (or 0). Messages / energy changes go through the VM. */
int acl_chest_toggle(AclVM *vm, const Mp2 *mp2, int thing, int *content);
/* 1 when the whole handler list of the room parses to exactly its length (the rule the Python parser uses) */
int acl_room_parse_ok(const Mp2 *mp2, int room);

/* same for the handler list of a thing record */
int acl_thing_parse_ok(const Mp2 *mp2, int thing);

/* number of operand bytes of opcode `op` (-1 = structured / unknown) */
int acl_oplen(int op);

#endif
