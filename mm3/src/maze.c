#include "maze.h"

#include <stdlib.h>
#include <string.h>

int mm3_page_load(Mm3Page *page, const uint8_t *data, size_t len) {
	if (len < MM3_PAGE_SIZE)
		return -1;
	for (int i = 0; i < MM3_PAGE_DIM * MM3_PAGE_DIM; i++)
		page->walls[i] = (uint16_t)(data[i * 2] | (data[i * 2 + 1] << 8));
	memcpy(page->flags, data + 0x200, 256);
	memcpy(page->header, data + 0x300, 64);
	return 0;
}

unsigned mm3_page_wall(const Mm3Page *page, int x, int y, int side) {
	uint16_t w;
	if (x < 0 || x >= MM3_PAGE_DIM || y < 0 || y >= MM3_PAGE_DIM || side < 0 || side > 3)
		return 0;
	w = page->walls[y * MM3_PAGE_DIM + x];
	return (w >> (12 - 4 * side)) & 0xF;
}

int mm3_events_load(Mm3EventList *list, const uint8_t *data, size_t len) {
	size_t pos = 0, cap = 0;
	list->events = NULL;
	list->count = 0;
	while (pos < len) {
		unsigned n = data[pos];
		Mm3Event *ev;
		if (n == 0) { /* padding byte at the end of some files */
			pos++;
			continue;
		}
		if (n < 5 || pos + 1 + n > len)
			break;
		if (list->count == cap) {
			Mm3Event *grown;
			cap = cap ? cap * 2 : 64;
			grown = realloc(list->events, cap * sizeof(*grown));
			if (!grown) {
				mm3_events_free(list);
				return -1;
			}
			list->events = grown;
		}
		ev = &list->events[list->count++];
		ev->x = data[pos + 1];
		ev->y = data[pos + 2];
		ev->facing = data[pos + 3];
		ev->line = data[pos + 4];
		ev->opcode = data[pos + 5];
		ev->nargs = (uint8_t)(n - 5);
		ev->args = data + pos + 6;
		pos += 1 + n;
	}
	return 0;
}

void mm3_events_free(Mm3EventList *list) {
	free(list->events);
	list->events = NULL;
	list->count = 0;
}

unsigned mm3_events_at(const Mm3EventList *list, int x, int y, int facing, const Mm3Event **out, unsigned max) {
	unsigned found = 0;
	for (unsigned i = 0; i < list->count && found < max; i++) {
		const Mm3Event *e = &list->events[i];
		if (e->x == x && e->y == y && (e->facing == facing || e->facing == 4))
			out[found++] = e;
	}
	/* insertion sort by line (stable) */
	for (unsigned i = 1; i < found; i++) {
		const Mm3Event *e = out[i];
		unsigned j = i;
		while (j > 0 && out[j - 1]->line > e->line) {
			out[j] = out[j - 1];
			j--;
		}
		out[j] = e;
	}
	return found;
}

const char *mm3_text_string(const uint8_t *data, size_t len, unsigned index) {
	size_t pos = 0;
	while (pos < len) {
		if (index == 0)
			return (const char *)data + pos;
		while (pos < len && data[pos])
			pos++;
		pos++;
		index--;
	}
	return NULL;
}
