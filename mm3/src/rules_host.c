/* Host routines for the translated rules code (src/gen/rules_gen.c). */
#include <stdio.h>
#include <stdlib.h>
#include "recomp.h"
#include "rules_host.h"

static int dialog_answer;
void mm3_rules_set_dialog_answer(int answer) { dialog_answer = answer; }

/* Borland LXMUL@: dx:ax = dx:ax * cx:bx (32-bit multiply, low 32 bits) */
void host_LXMUL_AT(Cpu *c) {
	if (getenv("MM3_TRACE")) fprintf(stderr, "LXMUL@ dx:ax=%04X:%04X cx:bx=%04X:%04X\n", c->dx, c->ax, c->cx, c->bx);
	uint32_t a = ((uint32_t)c->dx << 16) | c->ax, b = ((uint32_t)c->cx << 16) | c->bx, r = a * b;
	c->ax = (uint16_t)r;
	c->dx = (uint16_t)(r >> 16);
}

/* sub_3CE7A: the yes/no (who will) dialog used by ifProc mode 44; the answer comes from the caller until the UI exists */
void host_sub_3CE7A(Cpu *c) { c->ax = (uint16_t)dialog_answer; }
