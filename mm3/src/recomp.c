#include "recomp.h"

uint8_t MEM[MEM_SIZE + 0x10000];

void recomp_bad_jump(const char *fn, unsigned seg, unsigned off) {
	fprintf(stderr, "%s: computed jump to unlabelled target %04X:%04X (linear %05X)\n", fn, seg, off, seg * 16 + off);
	abort();
}
