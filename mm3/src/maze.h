/* MM3 maze data: MAZEnn.DAT pages, MAZEnn.EVT event scripts, TEXTnn.MAZ strings (docs/data-files.md). */
#ifndef MM3_MAZE_H
#define MM3_MAZE_H

#include <stddef.h>
#include <stdint.h>

#define MM3_PAGE_SIZE 832
#define MM3_PAGE_DIM 16

/* Wall sides, in the order of the nibbles of a cell word from the top. */
enum { MM3_SIDE_N = 0, MM3_SIDE_E = 1, MM3_SIDE_S = 2, MM3_SIDE_W = 3 };

/* Header bytes (offset from 300h of the .DAT). */
enum {
	MM3_HDR_RUN_CHANCE = 0x07,
	MM3_HDR_NEIGHBOUR_SOUTH = 0x08, /* map id of the page when y > 15 */
	MM3_HDR_NEIGHBOUR_EAST = 0x09,  /* x > 15 */
	MM3_HDR_NEIGHBOUR_NORTH = 0x0A, /* y < 0 */
	MM3_HDR_NEIGHBOUR_WEST = 0x0B,  /* x < 0 */
	MM3_HDR_CAN_SAVE = 0x0C,
	MM3_HDR_LIT = 0x0D,
	MM3_HDR_CAN_REST = 0x0E,
	MM3_HDR_CAN_DISMISS = 0x0F,
	MM3_HDR_DOOR_LOCK = 0x11,
	MM3_HDR_TRAP_LOCK = 0x12,
	MM3_HDR_START_CELL = 0x13, /* low nibble x, high nibble y */
	MM3_HDR_TRAP_DAMAGE = 0x1E
};

typedef struct {
	uint16_t walls[MM3_PAGE_DIM * MM3_PAGE_DIM]; /* index y * 16 + x */
	uint8_t flags[MM3_PAGE_DIM * MM3_PAGE_DIM];
	uint8_t header[64];
} Mm3Page;

/* Parse a 832-byte .DAT member; 0 on success. */
int mm3_page_load(Mm3Page *page, const uint8_t *data, size_t len);

/* Wall nibble of `side` (MM3_SIDE_*) of the cell: low 3 bits = style (0 = open), bit 3 = flag. */
unsigned mm3_page_wall(const Mm3Page *page, int x, int y, int side);

typedef struct {
	uint8_t x, y;
	uint8_t facing; /* 0-3, 4 = any */
	uint8_t line;
	uint8_t opcode;
	uint8_t nargs;
	const uint8_t *args; /* points into the source buffer */
} Mm3Event;

typedef struct {
	Mm3Event *events;
	unsigned count;
} Mm3EventList;

/* Parse a .EVT member.  The buffer must outlive the list (operands point into it). */
int mm3_events_load(Mm3EventList *list, const uint8_t *data, size_t len);
void mm3_events_free(Mm3EventList *list);

/* Events of a square, in line order.  Fills `out` (up to max), returns the number found. */
unsigned mm3_events_at(const Mm3EventList *list, int x, int y, int facing, const Mm3Event **out, unsigned max);

/* TEXTnn.MAZ: NUL-terminated strings.  Returns the string with the given index, or NULL. */
const char *mm3_text_string(const uint8_t *data, size_t len, unsigned index);

#endif
