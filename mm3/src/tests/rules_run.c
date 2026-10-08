/* rules_run IN.dg OUT.dg FUNC WORD...   -- run one translated rules routine (cdecl, far) on a DGROUP snapshot with the given word
 * arguments; prints "ax=XXXX dx=XXXX".  The dialog answer (ifProc mode 44) is 1.  Used by tools/mm3_rules_check.py. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../recomp.h"
#include "../rules_host.h"

extern const RecompEntry recomp_entries_rules_gen[];

int main(int argc, char **argv) {
	FILE *f;
	Cpu c;
	const RecompEntry *e;
	if (argc < 4) return 2;
	f = fopen(argv[1], "rb");
	if (!f || fread(DG, 1, 65536, f) != 65536) return 2;
	fclose(f);
	for (e = recomp_entries_rules_gen; e->name && strcmp(e->name, argv[3]); e++) {}
	if (!e->name) { fprintf(stderr, "no function %s\n", argv[3]); return 2; }
	memset(&c, 0, sizeof c);
	c.sp = 0xFF00; c.ds = DSEG;
	mm3_rules_set_dialog_answer(1);
	for (int i = argc - 1; i >= 4; i--) PUSH(&c, (uint16_t)strtoul(argv[i], NULL, 0));
	PUSH(&c, 0); PUSH(&c, 0);
	e->fn(&c);
	f = fopen(argv[2], "wb");
	fwrite(DG, 1, 65536, f);
	fclose(f);
	printf("ax=%04X dx=%04X\n", c.ax, c.dx);
	return 0;
}
