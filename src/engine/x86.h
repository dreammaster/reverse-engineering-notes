/*
 * x86.h - a small 8086 interpreter for the native routines of ROUTINEn.TAD (docs/formats.md 6.7); C port of tools/x86.py, verified against Unicorn
 * (tools/verify_x86.py) and against the Python reference (test vectors). Only the instructions found in the TAD files are implemented.
 */
#ifndef CADAVER_X86_H
#define CADAVER_X86_H

#include <stdint.h>

#define X86_MEM 0x100000

typedef struct X86 X86;
typedef void (*X86Hook)(X86 *c, void *user);

struct X86 {
	uint8_t *mem;                 /* 1 MB, owned by the caller */
	uint16_t ax, cx, dx, bx, sp, bp, si, di;
	uint16_t es, cs, ss, ds;
	uint16_t ip, fl;
	int err;                      /* non-zero: unsupported opcode / limit reached (the opcode is in err_op) */
	int err_op;
	/* hook: when execution reaches hook_cs:hook_ip the callback runs instead of the code there */
	uint16_t hook_cs, hook_ip;
	X86Hook hook;
	void *hook_user;
	/* port I/O (sound drivers): out(port, value, width), in(port, width) */
	void (*io_out)(void *user, unsigned port, unsigned v, int w);
	unsigned (*io_in)(void *user, unsigned port, int w);
	void *io_user;
};

void x86_init(X86 *c, uint8_t *mem);
uint8_t x86_rb(const X86 *c, unsigned seg, unsigned off);
uint16_t x86_rw(const X86 *c, unsigned seg, unsigned off);
void x86_wb(X86 *c, unsigned seg, unsigned off, unsigned v);
void x86_ww(X86 *c, unsigned seg, unsigned off, unsigned v);
void x86_push(X86 *c, unsigned v);
unsigned x86_pop(X86 *c);
/* Execute one instruction (or the hook). */
void x86_step(X86 *c);
/* Run until cs:ip == stop_cs:stop_ip; returns the number of instructions, or -1 on error / step limit. */
int x86_run(X86 *c, unsigned stop_cs, unsigned stop_ip, int max_steps);

#endif
