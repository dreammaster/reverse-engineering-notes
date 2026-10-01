/* The saved game state block (ROSTER.DAT + 1860h, 2052 bytes; docs/save-format.md) addressed by the
 * original DGROUP offsets, so event variables and effect bytes can be read exactly as the game does. */
#ifndef MM2_STATE_H
#define MM2_STATE_H

#include <stdint.h>

#define MM2_STATE_SIZE 2052

typedef struct {
	uint8_t blk[MM2_STATE_SIZE];
} Mm2State;

/* Pointer to the state byte that lives at DGROUP offset `dg`, or NULL if that address is not part
 * of the saved state. */
uint8_t *mm2_state_ptr(Mm2State *s, unsigned dg);

/* Event variable index (event opcodes 23/26) -> DGROUP offset (evt_var_addr 18E22); 0 = invalid. */
unsigned mm2_event_var_dgroup(int index);

/* Convenience accessors for well-known fields. */
unsigned mm2_state_era(const Mm2State *s);         /* g_era (03CA) */
unsigned mm2_state_party_size(const Mm2State *s);  /* g_party_size (0426) */
unsigned mm2_state_party_id(const Mm2State *s, int slot);   /* g_party_ids[slot] (0416), 0xFFFF = empty */

#endif
