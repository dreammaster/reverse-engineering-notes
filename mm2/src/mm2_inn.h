/* Inn: assembling the party (docs/party-commands.md; 1RETINN inn_menu 1C5C0). */
#ifndef MM2_INN_H
#define MM2_INN_H

#include "mm2_data.h"
#include "mm2_state.h"

#define MM2_FIRST_HIRELING 24
#define MM2_MAX_CHARS_IN_PARTY 6
#define MM2_MAX_HIRELINGS 2

typedef enum { MM2_INN_OK, MM2_INN_FULL, MM2_INN_ALREADY, MM2_INN_NOT_FOUND, MM2_INN_UNAVAILABLE } Mm2InnResult;

/* Party view over the saved state (g_party_ids / g_party_size). */
int mm2_party_size(const Mm2Roster *r);
int mm2_party_member(const Mm2Roster *r, int slot);   /* roster id or -1 */

/* Roster ids shown at the inn of `town` (0-4): characters whose location is that town, and the hirelings whose
 * quest variable is set and that live there.  Returns the count. */
int mm2_inn_list(const Mm2Roster *r, int town, int out[MM2_ROSTER_CHARS]);

Mm2InnResult mm2_inn_add(Mm2Roster *r, int rosterId);
Mm2InnResult mm2_inn_remove(Mm2Roster *r, int rosterId);
/* Moves a character to another town's inn (not allowed while in the party). */
Mm2InnResult mm2_inn_move(Mm2Roster *r, int rosterId, int town);
/* Leaving the inn: every party member now lives in `town`. */
void mm2_inn_leave(Mm2Roster *r, int town);

#endif
