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
	uint8_t vars[0x40];             /* ds:611.. */
	int c605;                       /* condition counter */
	int cur;                        /* current object (thing id) */
	int room_no;                    /* ds:232 */
	int reg;                        /* ds:704 */
	int disarm;
	int skipped;                    /* number of unsupported opcodes skipped so far */
	int (*rand_range)(int lo, int hi, void *user);
	void *user;
	uint8_t armed[128 + ACL_THINGS][64 / 8];   /* per record (rooms first, then things) and entry: 1 = disarmed */
} AclVM;

void acl_init(AclVM *vm, const Mp2 *mp2);
/* run all armed handlers of `event` in the handler list of a room record (kind 0) */
void acl_fire_room(AclVM *vm, const Mp2 *mp2, int room, int event);
/* 1 when the whole handler list of the room parses to exactly its length (the rule the Python parser uses) */
int acl_room_parse_ok(const Mp2 *mp2, int room);

/* number of operand bytes of opcode `op` (-1 = structured / unknown) */
int acl_oplen(int op);

#endif
